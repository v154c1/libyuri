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
	p["stream"]["Name of the stream to read"]="";
	return p;
}

NDIInput::NDIInput(log::Log &log_,core::pwThreadBase parent, const core::Parameters &parameters)
:core::IOThread(log_,parent,0,1,std::string("NDIInput")),
stream_("") {
	IOTHREAD_INIT(parameters)
	if (!NDIlib_initialize()) {
		log[log::fatal] << "Faile to initialize NDI input.";
		throw exception::InitializationFailed("Faile to initialize NDI input.");
    }
}

NDIInput::~NDIInput() {
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
		NDIlib_video_frame_v2_t video_frame;
		NDIlib_audio_frame_v2_t audio_frame;
		NDIlib_metadata_frame_t metadata_frame;
		yuri::format_t output_format;
		core::pRawVideoFrame frame;
		switch (NDIlib_recv_capture_v2(ndi_receiver, &video_frame, &audio_frame, &metadata_frame, 1000)) {
		// No data
		case NDIlib_frame_type_none:
			log[log::debug] << "No data received.";
			break;
		// Video data
		case NDIlib_frame_type_video:
			log[log::debug] << "Video data received: " << video_frame.xres << "x" << video_frame.yres;
			output_format = ndi_format_to_yuri(video_frame.FourCC);
			frame = core::RawVideoFrame::create_empty(output_format, {(uint32_t)video_frame.xres, (uint32_t)video_frame.yres}, true);
			std::copy(video_frame.p_data, video_frame.p_data + video_frame.yres * video_frame.line_stride_in_bytes, PLANE_DATA(frame, 0).begin());
			NDIlib_recv_free_video_v2(ndi_receiver, &video_frame);
			push_frame(0,frame);
			break;
		// Audio data
		case NDIlib_frame_type_audio:
			log[log::debug] << "Audio data received: " << audio_frame.no_samples << " samples.";
			NDIlib_recv_free_audio_v2(ndi_receiver, &audio_frame);
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
			)
		return true;
	return IOThread::set_param(param);


}

}
}
