/*
 * Rotate.cpp
 *
 *  Created on: 9.4.2013
 *      Author: neneko
 */


#include "Rotate.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_frame_types.h"
namespace yuri {
namespace rotate {

IOTHREAD_GENERATOR(Rotate)

MODULE_REGISTRATION_BEGIN("rotate")
		REGISTER_IOTHREAD("rotate",Rotate)
MODULE_REGISTRATION_END()

core::Parameters Rotate::configure()
{
	core::Parameters p = core::IOThread::configure();
	p.set_description("Rotate module.");
	p["angle"]["Angle in degrees CW to rotate (supported values are 0, 90, 180, 270)"]=90;
//	p->set_max_pipes(1,1);
	return p;
}


Rotate::Rotate(log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::SpecializedIOFilter<core::RawVideoFrame>(log_,parent, std::string("rotate")),angle_(90)
{
	IOTHREAD_INIT(parameters)
	set_supported_formats({core::raw_format::rgb24, core::raw_format::bgr24, core::raw_format::yuv444});
}

Rotate::~Rotate() noexcept
{
}

namespace {
core::pRawVideoFrame rotate(const core::pRawVideoFrame& frame, size_t angle) {
	core::pRawVideoFrame output;
	if (!frame) return output;
	const resolution_t res = frame->get_resolution();
	const size_t width = res.width;
	const size_t height = res.height;

	if (angle == 90 || angle==270) output = core::RawVideoFrame::create_empty(frame->get_format(), {height, width}, true);
	else if (angle == 180) output = core::RawVideoFrame::create_empty(frame->get_format(), res, true);
	else return output;
	const uint8_t * src = PLANE_RAW_DATA(frame,0);
	uint8_t * dest = PLANE_RAW_DATA(output,0);
	if (angle == 90) {
		for (size_t y = 0; y < height; ++y) {
			for (size_t x = 0; x < width; ++x) {
				const size_t new_x = height-y-1;
				const size_t new_y = x;
				const size_t pos = 3*(new_y*height+new_x);
				std::copy(src,src+3,&dest[pos]);
				src+=3;
			}
		}
	} else if (angle == 270) {
		for (size_t y = 0; y < height; ++y) {
			for (size_t x = 0; x < width; ++x) {
				const size_t new_x = y;
				const size_t new_y = width-x-1;
				const size_t pos = 3*(new_y*height+new_x);
				std::copy(src,src+3,&dest[pos]);
				src+=3;
			}
		}
	} else if (angle == 180) {
		for (size_t y = 0; y < height; ++y) {
			for (size_t x = 0; x < width; ++x) {
				const size_t new_x = width - x - 1;
				const size_t new_y = height - y - 1;
				const size_t pos = 3*(new_y*width+new_x);
				std::copy(src,src+3,&dest[pos]);
				src+=3;
			}
		}
	}
	output->copy_video_params(*frame);
	return output;
}
}

core::pFrame Rotate::do_special_single_step(core::pRawVideoFrame frame)
//bool Rotate::step()
{
//	if (!in[0]) return true;
//	core::pBasicFrame frame = in[0]->pop_frame();
//	if (!frame) return true;

	if(!angle_) return frame;
	else {
		if ((frame->get_format() != core::raw_format::rgb24) &&
				(frame->get_format() != core::raw_format::bgr24) &&
				(frame->get_format() != core::raw_format::yuv444)) {
			log[log::warning] << "Currently only 24bit RGB/BGR/YUV is supported.";
		} else {
//			core::pRawVideoFrame output = rotate(frame, angle_);
//			if (output) push_frame(0, output);
			return rotate(frame, angle_);
		}
	}
	return {};
}
bool Rotate::set_param(const core::Parameter &param)
{
	if (assign_parameters(param)
			(angle_, "angle"))
	{
		if (angle_ != 90 && angle_!=180 && angle_!=270) angle_ = 0;
		return true;
	}
	return core::SpecializedIOFilter<core::RawVideoFrame>::set_param(param);
}

} /* namespace rotate */
} /* namespace yuri */

