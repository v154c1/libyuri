/*!
 * @file 		PulseInput.cpp
 * @author 		Jiri Melnikov <jiri@melnikoff.org>
 * @date		24.9.2020
 * @copyright
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "PulseInput.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_audio_frame_types.h"
#include "yuri/core/frame/raw_audio_frame_params.h"

#include <stdio.h>
#include <string.h>
#include <memory>

namespace yuri {
namespace pulse {

IOTHREAD_GENERATOR(PulseInput)

core::Parameters PulseInput::configure() {
	core::Parameters p = core::IOThread::configure();
	p.set_description("PulseInput");

	std::string formats;
	for (const auto& fmt: core::raw_audio_format::formats()) {
		for (const auto& name: fmt.second.short_names) {
			formats += name + ", ";
		}
	}
	p["device"]["Pulse device to use"]="";
	p["channels"]["Channel to capture"]=2;
	p["sample_rate"]["Sample rate to capture"]=48000;
	p["latency"]["Max latency in bytes"]=128;
	p["format"]["Capture format. Valid values: (" + formats + ")"]="s16";
	return p;
}

#include "pulse_common.cpp"

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

PulseInput::PulseInput(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::IOThread(log_,parent,0,1,std::string("pulse_input")),
device_name_(""),format_(signed_16bit),channels_(2),latency_(128) {
	IOTHREAD_INIT(parameters)
}

PulseInput::~PulseInput() noexcept {
    destroy_pulse();
}

std::vector<core::InputDeviceInfo> PulseInput::enumerate() {
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

	for (auto &input : pulse_inputs) {
		core::InputDeviceInfo device;
		device.main_param_order = main_param_order;
		device.device_name = input.description;
		core::InputDeviceConfig cfg_base;
		cfg_base.params["index"]=input.index;
		cfg_base.params["name"]=input.name;
		device.configurations.push_back(std::move(cfg_base));
		devices.push_back(std::move(device));
	}

	close_pulse(pulse_loop);
	return devices;
}

void PulseInput::run()
{
	if (!init_pulse()) return;
    if (!set_format()) return;

	int yuri_format_bytes = get_yuri_format_bytes(format_);

	while(still_running()) {
		size_t len = pa_stream_readable_size(stream_);
		if (len == 0 || len == (size_t)-1) {
			usleep(1000*10);
			continue;
		}
		size_t rcv_len = 0;
		const void *rcv_data = nullptr;
		pa_threaded_mainloop_lock(pulse_loop_);
        if (pa_stream_peek(stream_, &rcv_data, &rcv_len) == 0) {
			if (rcv_data && rcv_len) {
				frames_ = rcv_len / (yuri_format_bytes * channels_);
				auto frame = core::RawAudioFrame::create_empty(format_, channels_, sample_rate_, frames_);
				memcpy(frame->data(), rcv_data, rcv_len);
				push_frame(0, frame);
				pa_stream_drop(stream_);
			} else if (!rcv_data && rcv_len) {
				pa_stream_drop(stream_);
				log[log::warning] << "Hole in the pulse buffer detected with length: " << rcv_len;
			} else if (!rcv_len) {
				log[log::warning] << "Cannot read audio samples from pulse.";
			}
		} else {
			log[log::warning] << "Error in pulse stream peak.";
		}
		pa_threaded_mainloop_unlock(pulse_loop_);
	}
}

bool PulseInput::set_format() {
	auto pulse_format = get_pulse_format(format_);
	if (pulse_format == PA_SAMPLE_INVALID) {
		log[log::error] << "Frame format not recognized by pulse.";
		return false;
	}

    pa_sample_spec ss = {
        pulse_format,					// .format
        sample_rate_,					// .rate
        static_cast<uint8_t>(channels_)	// .channels
    };

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
	if (!(stream_ = pa_stream_new(ctx_, "audio record", &ss, &map))) {
		log[log::error] << "Error while creating new pulse audio record.";
		return false;
	}

	uint32_t flags = 0;
	pa_buffer_attr bufattr;
	if (latency_ > 0) {
		memset(&bufattr, 0, sizeof(bufattr));
		bufattr.tlength = (uint32_t) latency_;
		bufattr.minreq = (uint32_t) 0;
		bufattr.maxlength = (uint32_t) -1;
		bufattr.prebuf = (uint32_t) -1;
		bufattr.fragsize = (uint32_t) latency_;
		flags |= PA_STREAM_ADJUST_LATENCY;
	}

   	if (pa_stream_connect_record(stream_, (device_name_.empty() ? nullptr : device_name_.c_str()), latency_ > 0 ? &bufattr : nullptr, static_cast<pa_stream_flags_t>(flags)) < 0) {
        log[log::error] << "Error while connecting stream to pulse record.";
		return false;
    }

	pa_threaded_mainloop_unlock(pulse_loop_);

	log[log::info] << "New format for pulse audio set.";

	return true;
}

bool PulseInput::init_pulse() {
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

void PulseInput::destroy_pulse() {
	if (pulse_loop_) close_pulse(pulse_loop_);
}

bool PulseInput::set_param(const core::Parameter& param) {
	if (assign_parameters(param)
			(device_name_, "device")
			(channels_, "channels")
			(sample_rate_, "sample_rate")
			.parsed<std::string>(format_, "format", core::raw_audio_format::parse_format)) {
		return true;
	} else return core::IOThread::set_param(param);
}

} /* namespace pulse */
} /* namespace yuri */
