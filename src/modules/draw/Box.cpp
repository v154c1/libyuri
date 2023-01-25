/*!
 * @file 		Box.cpp
 */

#include "Box.h"
#include "yuri/core/Module.h"
#include "yuri/core/utils/assign_events.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/raw_frame_traits.h"

namespace yuri {
namespace draw {

namespace {

using namespace core::raw_format;
const std::vector<format_t> supported_formats =
		{yuyv422, yvyu422, uyvy422, vyuy422, rgb24, bgr24, rgba32, bgra32};

template<uint8_t value, uint8_t value2>
void draw_frame_yuv(core::pRawVideoFrame& frame, size_t thickness) {
	auto& plane = PLANE_DATA(frame,0);
	const auto res = plane.get_resolution();
	auto iter = plane.begin();
	for (dimension_t line = 0; line < res.height; ++line) {
		if (line < thickness || res.height - line <= thickness) {
			for (dimension_t col = 0; col < res.width; ++col) {
				*iter++ = value;
				*iter++ = value2;
			}
		} else {
			iter += res.width*2;
		}
	}
}

template<uint8_t value, uint8_t value2, uint8_t value3>
void draw_frame_rgb(core::pRawVideoFrame& frame, size_t thickness) {
	auto& plane = PLANE_DATA(frame,0);
	const auto res = plane.get_resolution();
	auto iter = plane.begin();
	for (dimension_t line = 0; line < res.height; ++line) {
		if (line < thickness || res.height - line <= thickness) {
			for (dimension_t col = 0; col < res.width; ++col) {
				*iter++ = value;
				*iter++ = value2;
				*iter++ = value3;
			}
		} else {
			iter += res.width*3;
		}
	}
}

template<uint8_t value, uint8_t value2, uint8_t value3>
void draw_frame_rgba(core::pRawVideoFrame& frame, size_t thickness) {
	auto& plane = PLANE_DATA(frame,0);
	const auto res = plane.get_resolution();
	auto iter = plane.begin();
	for (dimension_t line = 0; line < res.height; ++line) {
		if (line < thickness || res.height - line <= thickness) {
			for (dimension_t col = 0; col < res.width; ++col) {
				*iter++ = value;
				*iter++ = value2;
				*iter++ = value3;
				*iter++ = 0;
			}
		} else {
			iter += res.width*4;
		}
	}
}


}

IOTHREAD_GENERATOR(Box)

core::Parameters Box::configure() {
	core::Parameters p = core::IOThread::configure();
	p.set_description("Box");
	p["thickness"]["Size of the box."]=5;
	return p;
}


Box::Box(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
base_type(log_,parent,std::string("box")), BasicEventConsumer(log), thickness_(5) {
	IOTHREAD_INIT(parameters)
	set_supported_formats(supported_formats);
}

Box::~Box() noexcept {
}

core::pFrame Box::do_special_single_step(core::pRawVideoFrame frame) {
	process_events();
	if (!thickness_) return frame;
	switch (frame->get_format()) {
		case core::raw_format::yuyv422:
		case core::raw_format::yvyu422:
			draw_frame_yuv<128,0>(frame, thickness_);
			break;
		case core::raw_format::uyvy422:
		case core::raw_format::vyuy422:
			draw_frame_yuv<0,128>(frame, thickness_);
			break;
		case core::raw_format::rgb24:
		case core::raw_format::bgr24:
			draw_frame_rgb<0,255,0>(frame, thickness_);
			break;
		case core::raw_format::rgba32:
		case core::raw_format::bgra32:
			draw_frame_rgba<0,255,0>(frame, thickness_);
			break;
	}
	return frame;
}


bool Box::set_param(const core::Parameter& param) {
if (assign_parameters(param)
			(thickness_, "thickness"))
		return true;
	return base_type::set_param(param);
}

bool Box::do_process_event(const std::string& event_name, const event::pBasicEvent& event) {
	if (assign_events(event_name, event)
			(thickness_, "thickness"))
		return true;
	return false;
}

} /* namespace draw */
} /* namespace yuri */
