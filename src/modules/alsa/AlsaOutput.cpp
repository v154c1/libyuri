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
#include "yuri/core/utils/irange.h"
namespace yuri {
namespace alsa {


IOTHREAD_GENERATOR(AlsaOutput)

core::Parameters AlsaOutput::configure()
{
	core::Parameters p = core::SpecializedIOFilter<core::RawAudioFrame>::configure();
	p.set_description("AlsaOutput");
	p["device"]["Alsa device to use"]="default";
	p["force_channels"]["Force number of channels for the output (set to 0 to automatic channel count)"]=0;
	p["mmap"]["Use mmap to access the device"]=false;
	p["buffer_size"]["Buffer size"]=24000;
    p["period_size"]["Period size"]=6000;
    p["periods"]["Periods"]=4;
	return p;
}

#include "alsa_common.cpp"

AlsaOutput::AlsaOutput(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::SpecializedIOFilter<core::RawAudioFrame>(log_,parent, std::string("alsa_output")),
format_(0),device_name_("default"),channels_(0),sampling_rate_(0),forced_channels_(0),buffer_size_{24000},
period_size_{6000},periods_{4},use_mmap_{false},handle_(0)
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
			(!forced_channels_ && (frame->get_channel_count() != channels_)) ||
			(forced_channels_ && (forced_channels_ != channels_));
}

namespace {
const uint8_t* write_data(log::Log& log, const uint8_t* start, const uint8_t* end, snd_pcm_t* handle, int timeout, size_t frame_size, bool use_mmap)
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
		if (!use_mmap) {
            write_frames = snd_pcm_writei(handle, reinterpret_cast<const void *>(start), write_frames);
        } else {
            write_frames = snd_pcm_mmap_writei(handle, reinterpret_cast<const void *>(start), write_frames);
		}
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

	const uint8_t* start = nullptr;
	const auto channel_size = frame->get_sample_size() / frame->get_channel_count() / 8;
	const auto data_size = channels_ * channel_size * frame->get_sample_count();
	
	if (channels_ != frame->get_channel_count()) {
		channel_buffer_.resize(frame->size() * channels_ / frame->get_channel_count());		
		uint8_t* buf_pos = &channel_buffer_[0];
		start = buf_pos;
		auto frame_pos = frame->data();
		const auto copy_channels = std::min(frame->get_channel_count(), channels_);
		const auto skip_channels = frame->get_channel_count() - copy_channels;
		const auto fill_channels = channels_ - copy_channels;
		
		
		for (auto i: irange(frame->get_sample_count())) {
			(void)i;
			for (auto x: irange(copy_channels * channel_size)) {
				(void)x;
				*buf_pos++ = *frame_pos++;
			}
			frame_pos += skip_channels * channel_size;
			for (auto x: irange(fill_channels * channel_size)) {
				(void)x;
				*buf_pos++ = 0;
			}
		}
	} else {
		// The simple case when we simple copy the data to sound card
		start = frame->data();
	}
	const uint8_t* end = start + data_size;
	while (start != end && still_running()) {
		start = write_data(log, start, end, handle_, static_cast<int>(get_latency().value/1000), channel_size * channels_, use_mmap_);
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
						"Failed to open device for playback")) return false;
		log[log::info] << "Device " << device_name_ << " opened";
	}
	format_ = 0;


	snd_pcm_format_t fmt = get_alsa_format(frame->get_format());
	if (fmt == SND_PCM_FORMAT_UNKNOWN) {
		log[log::warning] << "Received frame in unsupported format";
		return false;
	}

	channels_ = forced_channels_?forced_channels_:frame->get_channel_count();
	sampling_rate_ = frame->get_sampling_frequency();

	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;

	if(!error_call(snd_pcm_hw_params_malloc (&hw_params),
					"Failed to allocate HW params")) return false;
	if(!error_call(snd_pcm_hw_params_any (handle_, hw_params),
			"Failed to initialize HW params")) return false;

	if (!use_mmap_) {
        // Set access type to interleaved
        if (!error_call(snd_pcm_hw_params_set_access(handle_, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED),
                        "Failed to set access type"))
            return false;
    } else {
        if (!error_call(snd_pcm_hw_params_set_access(handle_, hw_params, SND_PCM_ACCESS_MMAP_INTERLEAVED),
                        "Failed to set access type"))
            return false;
	}

	if(!error_call(snd_pcm_hw_params_set_format (handle_, hw_params, fmt),
				"Failed to set format")) return false;

	int dir = 0;

	if(!error_call(snd_pcm_hw_params_set_rate_resample(handle_, hw_params, 1),
					"Failed to set resampling")) return false;


	if(!error_call(snd_pcm_hw_params_set_buffer_size(handle_, hw_params, buffer_size_),
		"Failed to set buffer size")) return false;
	if(!error_call(snd_pcm_hw_params_set_period_size(handle_, hw_params, period_size_, 0),
		"Failed to set period size")) return false;
	if(!error_call(snd_pcm_hw_params_set_periods(handle_, hw_params, periods_, 0),
		"Failed to set periods")) return false;

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
	if (assign_parameters(param) //
		(device_name_, "device") //
		(forced_channels_, "force_channels") //
		(buffer_size_, "buffer_size") //
        (period_size_, "period_size") //
        (periods_, "periods") //
		(use_mmap_, "mmap")) {
		return true;
	}
	return core::SpecializedIOFilter<core::RawAudioFrame>::set_param(param);
}

} /* namespace alsa */
} /* namespace yuri */
