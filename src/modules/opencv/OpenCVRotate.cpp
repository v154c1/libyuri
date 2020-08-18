/*!
 * @file 		OpenCV.cpp
 * @author 		Jiri Melnikov
 * @date 		15.5.2015
 * @date		16.5.2015
 * @copyright	CESNET, 2015
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "OpenCVRotate.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/utils/assign_events.h"
#include <cmath>
namespace yuri {
namespace opencv {

IOTHREAD_GENERATOR(OpenCVRotate)


core::Parameters OpenCVRotate::configure()
{
	core::Parameters p = base_type::configure();
	p.set_description("Rotate module.");
	p["angle"]["Angle in degrees CW to rotate"]=45;
	p["color"]["Background color"]=core::color_t::create_rgb(0, 0, 0);
	return p;
}


OpenCVRotate::OpenCVRotate(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
base_type(log_,parent,std::string("opencv_rotate")), event::BasicEventConsumer(log),
angle_(45)
{
	IOTHREAD_INIT(parameters)
	set_supported_formats({core::raw_format::rgba32});
}

OpenCVRotate::~OpenCVRotate() noexcept
{
}

namespace {
core::pRawVideoFrame rotate(const core::pRawVideoFrame& frame, double angle, const core::color_t& color) {
	const size_t width = frame->get_width();
	const size_t height = frame->get_height();
	cv::Mat in_mat(height,width,CV_8UC4,PLANE_RAW_DATA(frame,0));
	cv::Mat out_mat(height,width,CV_8UC4);
    size_t len = std::max(height, width);
    cv::Point2f pt(len/2.0f, len/2.0f);
    cv::Mat r = cv::getRotationMatrix2D(pt, angle, 1.0);
    cv::warpAffine(in_mat, out_mat, r, cv::Size(len, len), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(color.r(), color.g(), color.b(), color.a()));
    core::pRawVideoFrame output = core::RawVideoFrame::create_empty(core::raw_format::rgba32,
                                            {static_cast<dimension_t>(out_mat.cols), static_cast<dimension_t>(out_mat.rows)},
											out_mat.data,
											out_mat.total() * out_mat.elemSize());
    return output;
}
}

core::pFrame OpenCVRotate::do_special_single_step(core::pRawVideoFrame frame)
{
	process_events();
	if (frame->get_format() != core::raw_format::rgba32) {
		log[log::warning] << "Currently only 32bit RGBA is supported.";
		return frame;
	} else {
		if(!angle_)	return frame;
		return rotate(frame, angle_, color_);
	}
}

bool OpenCVRotate::set_param(const core::Parameter& param)
{
	if (assign_parameters(param)
			(angle_, "angle")
			(color_, "color"))
	{
		if (angle_ < 0 || angle_ > 360) angle_ = std::abs(fmod(angle_,360));
		return true;
	}
	return base_type::set_param(param);
}

bool OpenCVRotate::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
	if (assign_events(event_name, event)
			(angle_, 	"angle")
			(color_, 	"color"))
		return true;
	return false;
}

} /* namespace dummy_module */
} /* namespace yuri */
