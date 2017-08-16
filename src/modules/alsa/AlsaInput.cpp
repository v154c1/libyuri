/*!
 * @file 		AlsaInput.cpp
 * @author 		Jiri Melnikov <jiri@melnikoff.org>
 * @date		1.10.2017
 * @copyright
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "AlsaInput.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_audio_frame_types.h"
#include "yuri/core/frame/raw_audio_frame_params.h"

#include <memory>

namespace yuri {
namespace alsa {

IOTHREAD_GENERATOR(AlsaInput)

#include "alsa_common.cpp"

namespace {

unsigned int get_yuri_format_bytes(format_t fmt) {
	try {
		const auto& fi = core::raw_audio_format::get_format_info(fmt);
		return fi.bits_per_sample / 8;
	}
	catch (std::runtime_error&) {
		// This should never happen, but let's return a safe value'
		return 4;
	}
}
}

core::Parameters AlsaInput::configure()
{
	core::Parameters p = core::IOThread::configure();
	p.set_description("AlsaInput");

	std::string formats;
	for (const auto& fmt: core::raw_audio_format::formats()) {
		for (const auto& name: fmt.second.short_names) {
			formats += name + ", ";
		}
	}
	p["device"]["Alsa device to use"]="default";
	p["channels"]["Channel to capture"]=2;
	p["sample_rate"]["Sample rate to capture"]=48000;
	p["frames"]["Count of frames captured in one run"]=128;
	p["format"]["Capture format. Valid values: (" + formats + ")"]="s16";
	return p;
}

AlsaInput::AlsaInput(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::IOThread(log_,parent,0,1,std::string("alsa_input")),
format_(signed_16bit),device_name_("default"),channels_(2),sample_rate_(48000),frames_(128),frames_max_size_(0),handle_(0)
{
	IOTHREAD_INIT(parameters)
}

AlsaInput::~AlsaInput() noexcept
{
	if (handle_) error_call(snd_pcm_close (handle_), 	"Failed to close the device");
}

void AlsaInput::run()
{
	if (!init_alsa()) return;

	int yuri_format_bytes = get_yuri_format_bytes(format_);

	while(still_running()) {

		int rc = snd_pcm_readi(handle_, &data_[0], frames_);

		if (rc == -EPIPE) {
			log[log::info] << "Overrun occurred";
			snd_pcm_prepare(handle_);
		} else if (rc < 0) {
			log[log::warning] << "Error while read: " << snd_strerror(rc);
		} else if ((unsigned int)rc != frames_) {
			log[log::info] << "Short read, read only " << rc << " frames";
		}

		if(rc > 0) {
			auto frame = core::RawAudioFrame::create_empty(format_, channels_, sample_rate_, rc);
			unsigned int bytes = rc * yuri_format_bytes * channels_;
			memcpy(frame->data(), &data_[0], bytes);
			push_frame(0, frame);
		}
	}
}

bool AlsaInput::error_call(int ret, const std::string& msg)
{
	if (ret!=0) {
		log[log::warning] << msg;
		return false;
	}
	return true;
}
bool AlsaInput::init_alsa()
{
	if (handle_)
		if(!error_call(snd_pcm_close (handle_), "Failed to close the device")) return false;
	if(!error_call(snd_pcm_open (&handle_, device_name_.c_str(), SND_PCM_STREAM_CAPTURE, 0),
		"Failed to open device for capture")) return false;

	log[log::info] << "Device " << device_name_ << " opened";

	snd_pcm_format_t fmt = get_alsa_format(format_);
	if (fmt == SND_PCM_FORMAT_UNKNOWN) {
		log[log::warning] << "Requested format cannot be unknown.";
		return false;
	}

	frames_max_size_ = frames_ * channels_ * get_yuri_format_bytes(format_);
	data_.resize(frames_max_size_);

	snd_pcm_hw_params_t *hw_params;

	if(!error_call(snd_pcm_hw_params_malloc(&hw_params),
					"Failed to allocate HW params")) return false;

	if(!error_call(snd_pcm_hw_params_any(handle_, hw_params),
			"Failed to initialize HW params")) return false;

	// Set access type to interleaved
	if(!error_call(snd_pcm_hw_params_set_access(handle_, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED),
			"Failed to set access type")) return false;

	if(!error_call(snd_pcm_hw_params_set_format(handle_, hw_params, fmt),
				"Failed to set format")) return false;

	int dir = 0;

	if(!error_call(snd_pcm_hw_params_set_rate_resample(handle_, hw_params, 1),
					"Failed to set resampling")) return false;

	if(!error_call(snd_pcm_hw_params_set_rate_near(handle_, hw_params, &sample_rate_, &dir),
			"Failed to set sample rate")) return false;


	log[log::info] << "Initialized for " << sample_rate_ << " Hz";

	if(!error_call(snd_pcm_hw_params_set_channels(handle_, hw_params, channels_),
					"Failed to set number of channels")) return false;
	log[log::info] << "Initialized for " << static_cast<int>(channels_) << " channels";

	if(!error_call(snd_pcm_hw_params(handle_, hw_params),
				"Failed to set params")) return false;

	snd_pcm_hw_params_free(hw_params);

	if (!error_call(snd_pcm_prepare(handle_), "Failed to prepare PCM")) return false;

	return true;
}

bool AlsaInput::set_param(const core::Parameter& param) {
	if (assign_parameters(param)
			(device_name_, "device")
			(channels_, "channels")
			(sample_rate_, "sample_rate")
			(frames_, "frames")
			.parsed<std::string>(format_, "format", core::raw_audio_format::parse_format)) {
		return true;
	} else return core::IOThread::set_param(param);
}

} /* namespace alsa */
} /* namespace yuri */
