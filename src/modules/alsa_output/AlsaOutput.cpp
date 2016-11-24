/*!
 * @file 		AlsaOutput.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		22.10.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "AlsaOutput.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_audio_frame_types.h"
namespace yuri {
namespace alsa_output {


IOTHREAD_GENERATOR(AlsaOutput)

MODULE_REGISTRATION_BEGIN("alsa_output")
		REGISTER_IOTHREAD("alsa_output",AlsaOutput)
MODULE_REGISTRATION_END()

core::Parameters AlsaOutput::configure()
{
	core::Parameters p = core::SpecializedIOFilter<core::RawAudioFrame>::configure();
	p.set_description("AlsaOutput");
	p["device"]["Alsa device to use"]="default";
	return p;
}


namespace {
using namespace core::raw_audio_format;

std::map<format_t, snd_pcm_format_t> yuri_to_alsa_formats = {
		{unsigned_8bit, 	SND_PCM_FORMAT_U8},
		{unsigned_16bit, 	SND_PCM_FORMAT_U16_LE},
		{signed_16bit, 		SND_PCM_FORMAT_S16_LE},
		{unsigned_24bit, 	SND_PCM_FORMAT_U24_3LE},
		{signed_24bit, 		SND_PCM_FORMAT_S24_3LE},
		{unsigned_32bit, 	SND_PCM_FORMAT_U32_LE},
		{signed_32bit, 		SND_PCM_FORMAT_S32_LE},
		{float_32bit, 		SND_PCM_FORMAT_FLOAT_LE},

		{unsigned_16bit_be,	SND_PCM_FORMAT_U16_BE},
		{signed_16bit_be,	SND_PCM_FORMAT_S16_BE},
		{unsigned_24bit_be, SND_PCM_FORMAT_U24_3BE},
		{signed_24bit_be, 	SND_PCM_FORMAT_S24_3BE},
		{unsigned_32bit_be, SND_PCM_FORMAT_U32_BE},
		{signed_32bit_be,	SND_PCM_FORMAT_S32_BE},
		{float_32bit_be,	SND_PCM_FORMAT_FLOAT_BE},

};

snd_pcm_format_t get_alsa_format(format_t fmt) {
	auto it = yuri_to_alsa_formats.find(fmt);
	if (it == yuri_to_alsa_formats.end()) return SND_PCM_FORMAT_UNKNOWN;
	return it->second;
}


}



AlsaOutput::AlsaOutput(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::SpecializedIOFilter<core::RawAudioFrame>(log_,parent, std::string("alsa_output")),
format_(0),device_name_("default"),channels_(0),sampling_rate_(0)
{
	IOTHREAD_INIT(parameters)

}

AlsaOutput::~AlsaOutput() noexcept
{
	if (handle_) error_call(snd_pcm_close (handle_), 	"Failed to close the device");
}

bool AlsaOutput::is_different_format(const core::pRawAudioFrame& frame)
{
	return (frame->get_format() != format_) ||
			(frame->get_sampling_frequency() != sampling_rate_) ||
			(frame->get_channel_count() != channels_);
}

namespace {
const uint8_t* write_data(log::Log& log, const uint8_t* start, const uint8_t* end, snd_pcm_t* handle, int timeout, size_t frame_size)
{
	if (!snd_pcm_wait(handle, timeout)) {
		log[log::verbose_debug] << "Device busy";
		return start;
	}
	const snd_pcm_sframes_t avail_frames = (end - start) / frame_size;
	const snd_pcm_sframes_t frames_free = snd_pcm_avail(handle);
	snd_pcm_sframes_t write_frames = std::min(frames_free, avail_frames);
	if (write_frames > 0) {
		log[log::verbose_debug] << "Writing " << write_frames << " samples. Available was " << frames_free << ", I received " << avail_frames;
		write_frames = snd_pcm_writei(handle, reinterpret_cast<const void*>(start), write_frames);
		log[log::verbose_debug] << "Written " << write_frames << " frames";
	}
	if (write_frames < 0) {
		int ret = 0;
		if (write_frames == -EPIPE) {
			log[log::warning] << "AlsaDevice underrun! Recovering";
			ret = snd_pcm_recover(handle,write_frames,1);
		} else {
			log[log::warning] << "AlsaDevice write error, trying to recover";
			ret = snd_pcm_recover(handle,write_frames,0);
		}
		if (ret<0) {
			log[log::warning] << "Failed to recover from alsa error!";
			return end; // This is probably fatal, so no need to care about loosing few frames.
		}
	} else {
		return start + (write_frames * frame_size);
	}
	return start;

}

}

core::pFrame AlsaOutput::do_special_single_step(core::pRawAudioFrame frame)
{
	if (is_different_format(frame)) {
		if (!init_alsa(frame)) return {};
	}
	log[log::verbose_debug] << "Received frame with " << frame->get_sample_count() << " samples";
	if (!handle_) return {};

	const uint8_t* start = frame->data();
	const uint8_t* end = start+frame->size();
	while (start != end && still_running()) {
		start = write_data(log, start, end, handle_, static_cast<int>(get_latency().value/1000), frame->get_sample_size()/8);
	}
	return {};
}

bool AlsaOutput::error_call(int ret, const std::string& msg)
{
	if (ret!=0) {
		log[log::warning] << msg;
		return false;
	}
	return true;
}
bool AlsaOutput::init_alsa(const core::pRawAudioFrame& frame)
{
	if (!handle_) {
		if(!error_call(snd_pcm_open (&handle_, device_name_.c_str(), SND_PCM_STREAM_PLAYBACK, 0),
						"Failed to open device for capture")) return false;
		log[log::info] << "Device " << device_name_ << " opened";
	}
	format_ = 0;


	snd_pcm_format_t fmt = get_alsa_format(frame->get_format());
	if (fmt == SND_PCM_FORMAT_UNKNOWN) {
		log[log::warning] << "Received frame in unsupported format";
		return false;
	}

	channels_ = frame->get_channel_count();
	sampling_rate_ = frame->get_sampling_frequency();

	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;

	if(!error_call(snd_pcm_hw_params_malloc (&hw_params),
					"Failed to allocate HW params")) return false;
	if(!error_call(snd_pcm_hw_params_any (handle_, hw_params),
			"Failed to initialize HW params")) return false;

	// Set access type to interleaved
	if(!error_call(snd_pcm_hw_params_set_access (handle_, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED),
			"Failed to set access type")) return false;

	if(!error_call(snd_pcm_hw_params_set_format (handle_, hw_params, fmt),
				"Failed to set format")) return false;

	int dir = 0;

	if(!error_call(snd_pcm_hw_params_set_rate_resample(handle_, hw_params, 1),
					"Failed to set resampling")) return false;


	if(!error_call(snd_pcm_hw_params_set_rate_near (handle_, hw_params, &sampling_rate_, &dir),
			"Failed to set sample rate")) return false;


	log[log::info] << "Initialized for " << sampling_rate_ << " Hz";

	if(!error_call(snd_pcm_hw_params_set_channels (handle_, hw_params, channels_),
					"Failed to set number of channels")) return false;
	log[log::info] << "Initialized for " << static_cast<int>(channels_) << " channels";

	if(!error_call(snd_pcm_hw_params (handle_, hw_params),
				"Failed to set params")) return false;


	snd_pcm_hw_params_free (hw_params);


	if(!error_call(snd_pcm_sw_params_malloc (&sw_params),
			"cannot allocate software parameters structure")) return false;
	if(!error_call(snd_pcm_sw_params_current (handle_, sw_params),
			"cannot initialize software parameters structure")) return false;
	if(!error_call(snd_pcm_sw_params_set_avail_min (handle_, sw_params, 4096)
			,"cannot set minimum available count")) return false;
	if(!error_call(snd_pcm_sw_params_set_start_threshold (handle_, sw_params, 0U),
				"cannot set start mode")) return false;
	if(!error_call(snd_pcm_sw_params (handle_, sw_params),
				"cannot set software parameters")) return false;

	snd_pcm_sw_params_free (sw_params);

	if (!error_call(snd_pcm_prepare (handle_), "Failed to prepare PCM")) return false;

	format_ = frame->get_format();
	return true;
}
bool AlsaOutput::set_param(const core::Parameter& param)
{
	if (param.get_name() == "device") {
		device_name_ = param.get<std::string>();
	}/* else if (param.get_name() == "channels") {
		channels_ = param.get<std::string>();
	} else if (param.get_name() == "sample_rate") {
		sampling_rate_ = param.get<std::string>();
	}*/ else return core::SpecializedIOFilter<core::RawAudioFrame>::set_param(param);
	return true;
}

} /* namespace alsa_output */
} /* namespace yuri */
