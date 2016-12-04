/*!
 * @file 		common.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		24. 11. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "common.h"
#include "yuri/core/frame/raw_frame_types.h"
#include <numeric>
namespace yuri {
namespace color_picker {

namespace {
template<typename T, size_t size>
std::array<T, size> average_simple(const core::pRawVideoFrame& frame, const geometry_t& rect, size_t line_size)
{
	const size_t pixels = rect.width * rect.height;
	using pixel_type = std::array<T, size>;
	using acc_type = std::array<size_t, size>;
	pixel_type pixel_out;
	if (pixels) {
		acc_type vals;
		std::fill(vals.begin(), vals.end(), 0);
		auto it = reinterpret_cast<pixel_type *>(PLANE_RAW_DATA(frame,0))+rect.y*line_size + rect.x;
		for (dimension_t line = 0; line < rect.height; ++line) {
			vals = std::accumulate(it,it+rect.width,vals,[](const acc_type& a, const pixel_type& b)
					{
						acc_type acc;
						std::transform(a.begin(), a.end(), b.begin(), acc.begin(),[](const size_t&x, const T&y){return x+y;});
						return acc;
					}
			);
			it+=line_size;
		}
		std::transform(vals.begin(), vals.end(), pixel_out.begin(), [pixels](const size_t& x){return static_cast<T>(x/pixels);});
	} else {
		std::fill(pixel_out.begin(), pixel_out.end(), 0);
	}
	return pixel_out;
}

template<typename T, size_t size>
core::pRawVideoFrame draw_color(core::pRawVideoFrame frame, const geometry_t& rect, size_t line_size, const std::array<T, size>& avg)
{
	using pixel_type = std::array<T, size>;
	auto out_frame = get_frame_unique(frame);//std::dynamic_pointer_cast<core::RawVideoFrame>(frame->get_copy());
	auto it_out = reinterpret_cast<pixel_type *>(PLANE_RAW_DATA(out_frame,0))+rect.y*line_size+ rect.x;
	for (dimension_t line = 0; line < rect.height; ++line) {
		std::fill(it_out,it_out+rect.width,avg);
		it_out+=line_size;
	}
	return out_frame;
}

template<format_t>
struct get_color {};

#include "color_kernels.impl"


template<typename T, size_t size, format_t fmt>
std::tuple<core::pRawVideoFrame, core::color_t> process_colors(core::pRawVideoFrame frame, const geometry_t& rect, bool draw)
{
	const auto line_size = frame->get_resolution().width;
	const auto avg = average_simple<T, size>(frame, rect, line_size);
//	core::pRawVideoFrame frame_out;
	if (draw) {
		frame = draw_color(std::move(frame), rect, line_size, avg);
	} //else frame_out = frame;
	return std::make_tuple(std::move(frame), get_color<fmt>::eval(avg));
}


template<typename T, format_t fmt>
std::tuple<core::pRawVideoFrame, core::color_t> process_colors_yuv(core::pRawVideoFrame frame, geometry_t rect, bool draw)
{
	const auto line_size = frame->get_resolution().width /2;
	rect.width/=2; rect.x/=2;
	auto avg = average_simple<T, 4>(frame, rect, line_size);
//	core::pRawVideoFrame frame_out;
	if (draw) {
		frame = draw_color<T, 4>(std::move(frame), rect, line_size, avg);
//	} else {
//		frame_out = frame;
	}
	return std::make_tuple(std::move(frame), get_color<fmt>::eval(avg));
}


}




std::tuple<core::pRawVideoFrame, core::color_t>
process_rect(core::pRawVideoFrame frame, const geometry_t& geometry, bool show_color)
{
	using namespace core::raw_format;
	const format_t format = frame->get_format();
	const resolution_t resolution = frame->get_resolution();
//	core::pRawVideoFrame out_frame = frame;
	const auto rect = intersection(resolution, geometry);
//	core::color_t color;


	switch (format) {
		case rgb24:
			return process_colors<uint8_t, 3, rgb24>(std::move(frame), rect, show_color);
			break;
		case bgr24:
			return process_colors<uint8_t, 3, bgr24>(std::move(frame), rect, show_color);
			break;
		case rgb48:
			return process_colors<uint16_t, 3, rgb48>(std::move(frame), rect, show_color);
			break;
		case bgr48:
			return process_colors<uint16_t, 3, bgr48>(std::move(frame), rect, show_color);
			break;

		case rgba32:
			return process_colors<uint8_t, 4, rgba32>(std::move(frame), rect, show_color);
			break;
		case bgra32:
			return process_colors<uint8_t, 4, bgra32>(std::move(frame), rect, show_color);
			break;
		case argb32:
			return process_colors<uint8_t, 4, argb32>(std::move(frame), rect, show_color);
			break;
		case abgr32:
			return process_colors<uint8_t, 4, abgr32>(std::move(frame), rect, show_color);
			break;

		case rgba64:
			return process_colors<uint16_t, 4, rgba64>(std::move(frame), rect, show_color);
			break;
		case bgra64:
			return process_colors<uint16_t, 4, bgra64>(std::move(frame), rect, show_color);
			break;
		case argb64:
			return process_colors<uint16_t, 4, argb64>(std::move(frame), rect, show_color);
			break;
		case abgr64:
			return process_colors<uint16_t, 4, abgr64>(std::move(frame), rect, show_color);
			break;

		case yuyv422:
			return process_colors_yuv<uint8_t, yuyv422>(std::move(frame), rect, show_color);
			break;
		case yvyu422:
			return process_colors_yuv<uint8_t, yvyu422>(std::move(frame), rect, show_color);
			break;
		case uyvy422:
			return process_colors_yuv<uint8_t, uyvy422>(std::move(frame), rect, show_color);
			break;
		case vyuy422:
			return process_colors_yuv<uint8_t, vyuy422>(std::move(frame), rect, show_color);
			break;
		case yuv444:
			return process_colors<uint8_t, 3, yuv444>(std::move(frame), rect, show_color);
			break;
		case ayuv4444:
			return process_colors<uint8_t, 4, ayuv4444>(std::move(frame), rect, show_color);
			break;
		case yuva4444:
			return process_colors<uint8_t, 4, yuva4444>(std::move(frame), rect, show_color);
			break;
		// Single component
		case r8:
			return process_colors<uint8_t, 1, r8>(std::move(frame), rect, show_color);
			break;
		case r16:
			return process_colors<uint16_t, 1, r16>(std::move(frame), rect, show_color);
			break;
		case g8:
			return process_colors<uint8_t, 1, g8>(std::move(frame), rect, show_color);
			break;
		case g16:
			return process_colors<uint16_t, 1, g16>(std::move(frame), rect, show_color);
			break;
		case b8:
			return process_colors<uint8_t, 1, b8>(std::move(frame), rect, show_color);
			break;
		case b16:
			return process_colors<uint16_t, 1, b16>(std::move(frame), rect, show_color);
			break;
		case y8:
			return process_colors<uint8_t, 1, y8>(std::move(frame), rect, show_color);
			break;
		case y16:
			return process_colors<uint16_t, 1, y16>(std::move(frame), rect, show_color);
			break;
		case u8:
			return process_colors<uint8_t, 1, u8>(std::move(frame), rect, show_color);
			break;
		case u16:
			return process_colors<uint16_t, 1, u16>(std::move(frame), rect, show_color);
			break;
		case v8:
			return process_colors<uint8_t, 1, v8>(std::move(frame), rect, show_color);
			break;
		case v16:
			return process_colors<uint16_t, 1, v16>(std::move(frame), rect, show_color);
			break;
		default:
			throw std::runtime_error("Unsupported color space");

	}

}

}
}


