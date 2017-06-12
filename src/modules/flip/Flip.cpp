/*!
 * @file 		Flip.cpp
 * @author 		Zdenek Travnicek
 * @date 		16.3.2012
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2012 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "Flip.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/utils/assign_events.h"
#include <cassert>
#include <algorithm>
#include <array>
namespace yuri {

namespace io {

IOTHREAD_GENERATOR(Flip)

MODULE_REGISTRATION_BEGIN("flip")
		REGISTER_IOTHREAD("flip",Flip)
MODULE_REGISTRATION_END()

core::Parameters Flip::configure()
{
	core::Parameters p = core::SpecializedIOFilter<core::RawVideoFrame>::configure();
	p["flip_x"]["flip x (around y axis)"]=true;
	p["flip_y"]["flip y (around X axis)"]=false;
	return p;
}


namespace {
bool verify_support(const core::raw_format::raw_format_t& fmt)
{
	// multiple planes should be implemented as well..
	if (fmt.planes.size() != 1) return false;
	const auto& plane= fmt.planes[0];
	if (plane.components.empty()) return false;
	const auto& depth = plane.bit_depth;
	if (depth.first % (depth.second * 8)) return false;

	return true;
}
}

Flip::Flip(log::Log &_log, core::pwThreadBase parent, const core::Parameters &parameters)
:core::SpecializedIOFilter<core::RawVideoFrame>(_log,parent,"flip"),event::BasicEventConsumer(log),
 flip_x_(true),flip_y_(false)
 {
	IOTHREAD_INIT(parameters)
	std::vector<format_t> supported_fmts;
	for (const auto& f: core::raw_format::formats()) {
		if (verify_support(f.second)) {
			supported_fmts.push_back(f.first);
			log[log::verbose_debug] << "Setting format " << f.second.name << " as supported";
		}
	}
	set_supported_formats(supported_fmts);

}

Flip::~Flip() noexcept {

}


namespace {


template<int bpp>
struct cpy_helper {
	std::array<uint8_t, bpp> data;
};

struct cpy_helper_yuyv {
	std::array<uint8_t, 4> data;
	cpy_helper_yuyv& operator=(const cpy_helper_yuyv& rhs) {
		data[0]=rhs.data[2];
		data[1]=rhs.data[1];
		data[2]=rhs.data[0];
		data[3]=rhs.data[3];
		return *this;
	}
};
struct cpy_helper_uyvy {
	std::array<uint8_t, 4> data;
	cpy_helper_uyvy& operator=(const cpy_helper_uyvy& rhs) {
		data[0]=rhs.data[0];
		data[1]=rhs.data[3];
		data[2]=rhs.data[2];
		data[3]=rhs.data[1];
		return *this;
	}
};

template<class T>
struct flip_line_conv {
void operator()(const uint8_t* src, const uint8_t* src_end, uint8_t* dest)
{
	const T* s0 = reinterpret_cast<const T*>(src);
	const T* s1 = reinterpret_cast<const T*>(src_end);
	T* d0 = reinterpret_cast<T*>(dest);
	std::reverse_copy(s0, s1, d0);
}
};





template<class T, class F>
void process_lines(const uint8_t* src, uint8_t* dest, size_t lines, size_t line_size, size_t flip_y, F f = F())
{
	const size_t s_advance = flip_y?-line_size:line_size;
	if (flip_y) {
		src = src+(lines-1)*line_size;
	}
	const uint8_t* src_end = src + line_size;
	for (size_t line = 0;line < lines; ++line) {
		f(src, src_end, dest);
		src += s_advance;
		src_end += s_advance;
		dest += line_size;
	}
}

template<class T, template <class> class F>
void process_lines(const uint8_t* src, uint8_t* dest, size_t lines, size_t line_size, size_t flip_y)
{
	process_lines<T,F<T>>(src, dest, lines, line_size, flip_y);
}

void flip_dispatch(int yuv_pos, int bpp, bool flip_x, bool flip_y,
		size_t line_size, size_t lines,
		const uint8_t* in_ptr, uint8_t* out_ptr)
{
	if (!flip_x) {
		process_lines<cpy_helper_yuyv>(in_ptr, out_ptr, lines, line_size, flip_y,
				[](const uint8_t* src, const uint8_t* src_end, uint8_t* dest){std::copy(src, src_end, dest);});
	} else if (yuv_pos == 0) {
		process_lines<cpy_helper_yuyv, flip_line_conv>(in_ptr, out_ptr, lines, line_size, flip_y);
	} else if (yuv_pos == 1) {
		process_lines<cpy_helper_uyvy, flip_line_conv>(in_ptr, out_ptr, lines, line_size, flip_y);
	} else {
		assert(line_size % bpp == 0);
		switch (bpp) {
			case 1:process_lines<cpy_helper<1>, flip_line_conv>(in_ptr, out_ptr, lines, line_size, flip_y);break;
			case 2:process_lines<cpy_helper<2>, flip_line_conv>(in_ptr, out_ptr, lines, line_size, flip_y);break;
			case 3:process_lines<cpy_helper<3>, flip_line_conv>(in_ptr, out_ptr, lines, line_size, flip_y);break;
			case 4:process_lines<cpy_helper<4>, flip_line_conv>(in_ptr, out_ptr, lines, line_size, flip_y);break;
			case 5:process_lines<cpy_helper<5>, flip_line_conv>(in_ptr, out_ptr, lines, line_size, flip_y);break;
			case 6:process_lines<cpy_helper<6>, flip_line_conv>(in_ptr, out_ptr, lines, line_size, flip_y);break;
			case 7:process_lines<cpy_helper<7>, flip_line_conv>(in_ptr, out_ptr, lines, line_size, flip_y);break;
			case 8:process_lines<cpy_helper<8>, flip_line_conv>(in_ptr, out_ptr, lines, line_size, flip_y);break;
			default:break;
		}
	}
}

}

core::pFrame Flip::do_special_single_step(core::pRawVideoFrame frame)
{
	process_events();
	if (!flip_x_ && !flip_y_) return frame;
	size_t	w = frame->get_width();
	size_t	h = frame->get_height();



	const auto& fi = core::raw_format::get_format_info(frame->get_format());

	if (!verify_support(fi)) return {};

	const auto depth = fi.planes[0].bit_depth;
	const size_t bpp = depth.first/depth.second/8;
	const size_t line_length = w*bpp;
	int yuv_y_pos = -1;


	// Special cases for yuv 422 formats.
	if (frame->get_format()==core::raw_format::yuyv422 ||
		frame->get_format()==core::raw_format::yvyu422) {
		yuv_y_pos = 0;
	} else if (frame->get_format()==core::raw_format::uyvy422 ||
			frame->get_format()==core::raw_format::vyuy422 ) {
		yuv_y_pos = 1;
	}

	core::pRawVideoFrame frame_out = core::RawVideoFrame::create_empty(frame->get_format(), frame->get_resolution());

	const uint8_t * in_ptr =  PLANE_RAW_DATA(frame,0);
	uint8_t *out_ptr = PLANE_RAW_DATA(frame_out,0);

	flip_dispatch(yuv_y_pos, bpp, flip_x_, flip_y_, line_length, h, in_ptr, out_ptr);

	return frame_out;
}

bool Flip::set_param(const core::Parameter &parameter)
{
	if(assign_parameters(parameter)
			(flip_x_, "flip_x")
			(flip_y_, "flip_y"))
		return true;
	return core::SpecializedIOFilter<core::RawVideoFrame>::set_param(parameter);
}


bool Flip::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
	if (assign_events(event_name, event)
			(flip_x_, "flip_x")
			(flip_y_, "flip_y"))
		return true;
	return false;
}
}

}
