/*
 * NDIInput.cpp
 */

#include "NDIInput.h"

#include "yuri/core/Module.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/frame/RawAudioFrame.h"
#include "yuri/core/frame/raw_audio_frame_types.h"
#include "yuri/core/frame/raw_audio_frame_params.h"

#include "yuri/core/utils.h"

#include <cassert>

namespace yuri {
namespace ndi {

namespace {

using namespace yuri::core::raw_format;

std::map<NDIlib_FourCC_type_e, yuri::format_t> pixel_format_map = {
	{NDIlib_FourCC_type_UYVY,	uyvy422},
	{NDIlib_FourCC_type_BGRA,	bgra32},
	{NDIlib_FourCC_type_BGRX,	bgr24},
	{NDIlib_FourCC_type_RGBA,	rgba32},
	{NDIlib_FourCC_type_RGBX,	rgb24},
	{NDIlib_FourCC_type_UYVA,	yuva4444},
};

/* Not in use now.
NDIlib_FourCC_type_e yuri_format_to_ndi(format_t fmt) {
	for (auto f: pixel_format_map) {
		if (f.second == fmt) return f.first;
	}
	throw exception::Exception("No NDI format found.");
}
*/

format_t ndi_format_to_yuri (NDIlib_FourCC_type_e fmt) {
	auto it = pixel_format_map.find(fmt);
	if (it == pixel_format_map.end()) throw exception::Exception("No Yuri format found.");
	return it->second;
}

}

IOTHREAD_GENERATOR(NDIInput)

core::Parameters NDIInput::configure() {
	core::Parameters p = IOThread::configure();
	p["stream"]["Name of the stream to read."]="";
	p["audio"]["Set to true if audio should be received."]=false;
	return p;
}

NDIInput::NDIInput(log::Log &log_,core::pwThreadBase parent, const core::Parameters &parameters)
:core::IOThread(log_,parent,0,1,std::string("NDIInput")),
stream_(""),audio_enabled_(false),audio_pipe_(-1) {
	IOTHREAD_INIT(parameters)
	if (!NDIlib_initialize()) {
		log[log::fatal] << "Faile to initialize NDI input.";
		throw exception::InitializationFailed("Faile to initialize NDI input.");
    }
	// Audio pipe is for further multichannel implementation
	audio_pipe_=(audio_enabled_?1:-1);
	resize(0,1+(audio_enabled_?1:0));
}

NDIInput::~NDIInput() {
}

std::vector<core::InputDeviceInfo> NDIInput::enumerate() {
	// Returned devices
	std::vector<core::InputDeviceInfo> devices;
	std::vector<std::string> main_param_order = {"address", "name"};

	// Create a finder
	const NDIlib_find_create_t finder_desc;
	NDIlib_find_instance_t ndi_finder = NDIlib_find_create_v2(&finder_desc);
	if (!ndi_finder) {
		throw exception::InitializationFailed("Faile to initialize NDI fidner.");
	}

	// Search for the source on the network
	const NDIlib_source_t* sources = NULL;
	NDIlib_find_wait_for_sources(ndi_finder, 1000);
	uint32_t count;
	sources = NDIlib_find_get_current_sources(ndi_finder, &count);
	for (uint32_t i = 0; i < count; i++) {
		core::InputDeviceInfo device;
		device.main_param_order = main_param_order;
		device.device_name = sources[i].p_ndi_name;
		core::InputDeviceConfig cfg_base;
		cfg_base.params["address"]=sources[i].p_ip_address;
		cfg_base.params["name"]=sources[i].p_ndi_name;
		device.configurations.push_back(std::move(cfg_base));
		devices.push_back(std::move(device));
	}
	return devices;
}

void NDIInput::run() {
	// Create a finder
	const NDIlib_find_create_t finder_desc;
	NDIlib_find_instance_t ndi_finder = NDIlib_find_create_v2(&finder_desc);
	if (!ndi_finder) {
		log[log::fatal] << "Faile to initialize NDI fidner.";
		throw exception::InitializationFailed("Faile to initialize NDI fidner.");
	}

	// Search for the source on the network
	uint32_t stream_found = 0;
	const NDIlib_source_t* sources = NULL;
	while (still_running() && !stream_found) {
		NDIlib_find_wait_for_sources(ndi_finder, 1000);
		uint32_t count;
		sources = NDIlib_find_get_current_sources(ndi_finder, &count);
		for (uint32_t i = 0; i < count; i++) {
			log[log::info] << "Searching for stream: \"" << stream_ << "\" but found: \"" << sources[i].p_ndi_name << "\"";
			if (stream_ == sources[i].p_ndi_name)
				stream_found = (i+1);
		}
	}

	NDIlib_recv_create_v3_t receiver_desc;
	receiver_desc.source_to_connect_to = sources[stream_found-1];
	receiver_desc.p_ndi_name = "Yuri NDI receiver";
	receiver_desc.color_format = NDIlib_recv_color_format_e_UYVY_RGBA;
	receiver_desc.bandwidth = NDIlib_recv_bandwidth_highest;


	NDIlib_recv_instance_t ndi_receiver = NDIlib_recv_create_v3(&receiver_desc);
	if (!ndi_receiver) {
		log[log::fatal] << "Faile to initialize NDI receiver.";
		throw exception::InitializationFailed("Faile to initialize NDI receiver.");
	}

	// Destroy finder
	NDIlib_find_destroy(ndi_finder); 

	// We are now going to mark this source as being on program output for tally purposes (but not on preview)
	NDIlib_tally_t tally_state;
	tally_state.on_program = true;
	tally_state.on_preview = true;
	NDIlib_recv_set_tally(ndi_receiver, &tally_state);

	NDIlib_metadata_frame_t enable_hw_accel;
	enable_hw_accel.p_data = (char*)"<ndi_hwaccel enabled=\"true\"/>";
	NDIlib_recv_send_metadata(ndi_receiver, &enable_hw_accel);

	log[log::info] << "Receiving started";

	while (still_running()) {
		NDIlib_video_frame_v2_t n_video_frame;
		NDIlib_audio_frame_v2_t n_audio_frame;
		NDIlib_metadata_frame_t metadata_frame;
		NDIlib_audio_frame_interleaved_16s_t n_audio_frame_16bpp_interleaved;
		// Yuri Video
		yuri::format_t y_video_format;
		core::pRawVideoFrame y_video_frame;
		// Yuri Audio
		core::pRawAudioFrame y_audio_frame;
		// Receive
		switch (NDIlib_recv_capture_v2(ndi_receiver, &n_video_frame, &n_audio_frame, &metadata_frame, 1000)) {
		// No data
		case NDIlib_frame_type_none:
			log[log::debug] << "No data received.";
			break;
		// Video data
		case NDIlib_frame_type_video:
			log[log::debug] << "Video data received: " << n_video_frame.xres << "x" << n_video_frame.yres;
			y_video_format = ndi_format_to_yuri(n_video_frame.FourCC);
			y_video_frame = core::RawVideoFrame::create_empty(y_video_format, {(uint32_t)n_video_frame.xres, (uint32_t)n_video_frame.yres}, true);
			std::copy(n_video_frame.p_data, n_video_frame.p_data + n_video_frame.yres * n_video_frame.line_stride_in_bytes, PLANE_DATA(y_video_frame, 0).begin());
			NDIlib_recv_free_video_v2(ndi_receiver, &n_video_frame);
			push_frame(0,y_video_frame);
			break;
		// Audio data
		case NDIlib_frame_type_audio:
			if (audio_enabled_) {
				log[log::debug] << "Audio data received: " << n_audio_frame.no_samples << " samples, " << n_audio_frame.no_channels << " channels.";
				n_audio_frame_16bpp_interleaved.reference_level = 20;	// 20dB of headroom
				n_audio_frame_16bpp_interleaved.p_data = new short[n_audio_frame.no_samples*n_audio_frame.no_channels];
				// Convert it
				NDIlib_util_audio_to_interleaved_16s_v2(&n_audio_frame, &n_audio_frame_16bpp_interleaved);
				// Process data!
				y_audio_frame = core::RawAudioFrame::create_empty(core::raw_audio_format::signed_16bit, n_audio_frame.no_channels, n_audio_frame.sample_rate, n_audio_frame_16bpp_interleaved.p_data , n_audio_frame.no_samples * n_audio_frame.no_channels * 2);
				push_frame(audio_pipe_, y_audio_frame);
				// Free the interleaved audio data
				delete[] n_audio_frame_16bpp_interleaved.p_data;
			}
			NDIlib_recv_free_audio_v2(ndi_receiver, &n_audio_frame);
			break;
		// Meta data
		case NDIlib_frame_type_metadata:
			log[log::debug] << "Metadata received.";
			NDIlib_recv_free_metadata(ndi_receiver, &metadata_frame);
			break;
		// There is a status change on the receiver (e.g. new web interface)
		case NDIlib_frame_type_status_change:
			log[log::debug] << "Receiver connection status changed.";
			break;
		// Everything else
		default:
			break;
		}
	}

	log[log::info] << "Stopping receiver";

	// Get it out
	NDIlib_recv_destroy(ndi_receiver);
	NDIlib_destroy();
}


bool NDIInput::set_param(const core::Parameter &param) {
	if (assign_parameters(param)
			(stream_, "stream")
			(audio_enabled_, "audio")
			)
		return true;
	return IOThread::set_param(param);


}

}
}
