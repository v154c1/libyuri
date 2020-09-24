/*!
 * @file 		SplitPlanes.cpp
 * @author 		<Your name>
 * @date		31.03.2014
 * @copyright	Institute of Intermedia, 2013
 * 				Distributed BSD License
 *
 */

#include "SplitPlanes.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/raw_frame_params.h"

namespace yuri {
namespace split_planes {


IOTHREAD_GENERATOR(SplitPlanes)

MODULE_REGISTRATION_BEGIN("split_planes")
		REGISTER_IOTHREAD("split_planes",SplitPlanes)
MODULE_REGISTRATION_END()

core::Parameters SplitPlanes::configure()
{
	core::Parameters p = base_type::configure();
	p.set_description("SplitPlanes");
	p["keep_format"]["Keep original formats. Setting to to false will mark all outputs as Y"]=true;
	return p;
}


SplitPlanes::SplitPlanes(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
base_type(log_,parent,4,std::string("split_planes")),keep_format_(true)
{
	IOTHREAD_INIT(parameters)
//	set_supported_formats({core::raw_format::yuv444p, core::raw_format::yuv422p, core::raw_format::yuv420p});

}

SplitPlanes::~SplitPlanes() noexcept
{
}
namespace {
//using core::raw_format;
//std::map<format_t, std::vector<format_t> > plane_mapping = {
//		yuv422p
//
//
//};


template<format_t plane_type>
core::pFrame get_plane(const core::pRawVideoFrame& frame, size_t plane_index)
{
	auto f = core::RawVideoFrame::create_empty(plane_type, PLANE_DATA(frame, plane_index).get_resolution());
	assert(PLANE_SIZE(f,0)==PLANE_SIZE(frame,plane_index));
	std::copy(PLANE_DATA(frame, plane_index).begin(), PLANE_DATA(frame,plane_index).end(), PLANE_DATA(f,0).begin());
	return f;
}

template<format_t plane1, format_t plane2, format_t plane3>
std::vector<core::pFrame> split(core::pRawVideoFrame frame)
{
	std::vector<core::pFrame> outframes;
	outframes.push_back(get_plane<plane1>(frame, 0));
	outframes.push_back(get_plane<plane2>(frame, 1));
	outframes.push_back(get_plane<plane3>(frame, 2));
	return outframes;
}

}


std::vector<core::pFrame> SplitPlanes::do_special_step(std::tuple<core::pRawVideoFrame> frames)
{
	const auto frame = std::get<0>(frames);
	const format_t format = frame->get_format();
	const auto& fi = core::raw_format::get_format_info(format);
	if (fi.planes.size()<2) return {frame};
	using namespace core::raw_format;
	if (keep_format_) {
		switch(format) {
			case yuv444p:
			case yuv422p:
			case yuv420p:
			case yuv411p:
				return split<y8, u8, v8>(frame);
			case rgb24p:
				return split<r8, g8, b8>(frame);
			case bgr24p:
				return split<b8, g8, r8>(frame);
			case gbr24p:
				return split<g8, b8, r8>(frame);
//			case rgba32p:
//				return split<b8, g8, r8, alpha8>(frame);
//			case abgr32p:
//				return split<alpha8, b8, g8, r8>(frame);
			default:
				break;
		}
	} else {
		switch(format) {
			case yuv444p:
			case yuv422p:
			case yuv420p:
			case yuv411p:
			case rgb24p:
			case bgr24p:
			case gbr24p:
				return split<y8, y8, y8>(frame);
//			case rgba32p:
//			case abgr32p:
//				return split<y8, y8, y8, y8>(frame);
			default:
				break;
			}
	}
	return {};
}
bool SplitPlanes::set_param(const core::Parameter& param)
{
	if (param.get_name() == "keep_format") {
		keep_format_ = param.get<bool>();
	} else return base_type::set_param(param);
	return true;
}

} /* namespace split_planes */
} /* namespace yuri */
