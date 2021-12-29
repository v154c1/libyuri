/*!
 * @file 		PulseOutput.cpp
 * @author 		Jiri Melnikov <jiri@melnikoff.org>
 * @date		11.6.2019
 * @copyright	
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "PulseOutput.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_audio_frame_types.h"
#include "yuri/core/utils/irange.h"

#include <string.h>
#include <unistd.h>

namespace yuri {
namespace pulse {


IOTHREAD_GENERATOR(PulseOutput)

core::Parameters PulseOutput::configure() {
	core::Parameters p = core::SpecializedIOFilter<core::RawAudioFrame>::configure();
	p.set_description("PulseOutput");
	p["device"]["Pulse device to use"]="";
	p["force_channels"]["Force number of channels for the output (set to 0 to automatic channel count)"]=0;
	return p;
}

#include "pulse_common.cpp"

PulseOutput::PulseOutput(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::SpecializedIOFilter<core::RawAudioFrame>(log_,parent, std::string("pulse_output")),
device_name_(""),format_(0),channels_(0),forced_channels_(0),sample_rate_(0),pulse_ready_(0)
{
	IOTHREAD_INIT(parameters)
	init_pulse();
}

PulseOutput::~PulseOutput() noexcept {
	destroy_pulse();
}

std::vector<core::InputDeviceInfo> PulseOutput::enumerate() {
	// Returned devices
	std::vector<core::InputDeviceInfo> devices;
	std::vector<std::string> main_param_order = {"index","name"};
	// Pulse context and loop
	pa_context *ctx;
	pa_threaded_mainloop *pulse_loop;
	int pulse_ready = 0;

	// Request connect
	try	{
		ctx = get_pulse_connect(&pulse_loop, &pulse_ready);
	} catch(const std::exception& e) {
		return devices;
	}

	while (pulse_ready != 1) {
		if (pulse_ready == 2) {
			close_pulse(pulse_loop);
			return devices;
		}
		usleep(1000);
	}

    std::vector<pulse_device_t> pulse_outputs;
	std::vector<pulse_device_t> pulse_inputs;
	get_pulse_outputs(ctx, &pulse_outputs);
	get_pulse_inputs(ctx, &pulse_inputs);

	for (auto &output : pulse_outputs) {
		core::InputDeviceInfo device;
		device.main_param_order = main_param_order;
		device.device_name = output.description;
		core::InputDeviceConfig cfg_base;
		cfg_base.params["index"]=output.index;
		cfg_base.params["name"]=output.name;
		device.configurations.push_back(std::move(cfg_base));
		devices.push_back(std::move(device));
	}

	close_pulse(pulse_loop);
	return devices;
}



core::pFrame PulseOutput::do_special_single_step(core::pRawAudioFrame frame) {
	if (pulse_ready_ != 1)
		return {};

	if (is_different_format(frame))
		if (!set_format(frame)) return {};

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
	pa_threaded_mainloop_lock(pulse_loop_);
	if (pa_stream_write(stream_, start, data_size, nullptr, 0, PA_SEEK_RELATIVE) < 0) {
		log[log::warning] << "Not able to write data to pulse audio.";
	}
	pa_threaded_mainloop_unlock(pulse_loop_);
	return {};
}

bool PulseOutput::is_different_format(const core::pRawAudioFrame& frame) {
	return ((frame->get_format() != format_) ||
			(frame->get_sampling_frequency() != sample_rate_) ||
			(!forced_channels_ && (frame->get_channel_count() != channels_)) ||
			(forced_channels_ && (forced_channels_ != channels_)));
}

bool PulseOutput::set_format(const core::pRawAudioFrame& frame) {
	auto pulse_format = get_pulse_format(frame->get_format());
	if (pulse_format == PA_SAMPLE_INVALID) {
		log[log::error] << "Received frame in unsupported format";
		return false;
	}

	format_ = frame->get_format();
	channels_ = forced_channels_ ? forced_channels_ : frame->get_channel_count();
	if (channels_ > pulse_max_output_channels) {
		forced_channels_ = pulse_max_output_channels;
		channels_ = forced_channels_;
	}
	sample_rate_ = frame->get_sampling_frequency();

    pa_sample_spec ss = {
        pulse_format,					// .format
        sample_rate_,					// .rate
        static_cast<uint8_t>(channels_)	// .channels
    };

	log[log::info] << "Initialized for " << sample_rate_ << " Hz";
	log[log::info] << "Initialized for " << static_cast<int>(channels_) << " channels";

	if (!pa_sample_spec_valid(&ss)) {
        log[log::error] << "Unsupported sample type (format/rate/channels) by pulse audio.";
		return false;
    }

	struct pa_channel_map map;
	map.channels = 0;
	if (channels_ == 1)
		map.map[map.channels++] = PA_CHANNEL_POSITION_MONO;
	if (channels_ > 1) {
		map.map[map.channels++] = PA_CHANNEL_POSITION_FRONT_LEFT;
		map.map[map.channels++] = PA_CHANNEL_POSITION_FRONT_RIGHT;
	}
	if (channels_ > 2)
		map.map[map.channels++] = PA_CHANNEL_POSITION_SIDE_LEFT;
	if (channels_ > 3)
		map.map[map.channels++] = PA_CHANNEL_POSITION_SIDE_RIGHT;
	if (channels_ > 4)
		map.map[map.channels++] = PA_CHANNEL_POSITION_REAR_LEFT;
	if (channels_ > 5)
		map.map[map.channels++] = PA_CHANNEL_POSITION_REAR_RIGHT;

	if (!pa_channel_map_valid(&map)) {
		log[log::error] << "Unsupported sample channel map by pulse audio.";
		return false;
	}

	pa_threaded_mainloop_lock(pulse_loop_);
	if (!(stream_ = pa_stream_new(ctx_, "audio playback", &ss, &map))) {
		log[log::error] << "Error while creating new pulse audio playback.";
		return false;
	}

   	if (pa_stream_connect_playback(stream_, (device_name_.empty() ? nullptr : device_name_.c_str()), nullptr, PA_STREAM_START_UNMUTED, nullptr, nullptr) < 0) {
        log[log::error] << "Error while connecting stream to pulse playback.";
		return false;
    }

	pa_threaded_mainloop_unlock(pulse_loop_);

	log[log::info] << "New format for pulse audio set.";

	return true;
}

bool PulseOutput::init_pulse() {
	try {
		ctx_ = get_pulse_connect(&pulse_loop_, &pulse_ready_);
	} catch(const std::exception& e) {
		log[log::error] << "Pulse error: " << e.what();
		return false;
	}
	while (pulse_ready_ != 1) {
		if (pulse_ready_ == 2) {
			close_pulse(pulse_loop_);
			log[log::error] << "Pulse audio disconnected or in error state.";
			return false;
		}
		usleep(1000);
	}
	return true;
}

void PulseOutput::destroy_pulse() {
	if (pulse_loop_) close_pulse(pulse_loop_);
}

bool PulseOutput::set_param(const core::Parameter& param)
{
	if (assign_parameters(param) //
		(device_name_, "device") //
		(forced_channels_, "force_channels")) {
		return true;
	}
	return core::SpecializedIOFilter<core::RawAudioFrame>::set_param(param);
}

} /* namespace pulse */
} /* namespace yuri */
