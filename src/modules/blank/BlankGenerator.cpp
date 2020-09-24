/*!
 * @file 		BlankGenerator.cpp
 * @author 		Zdenek Travnicek
 * @date 		11.9.2010
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2010 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "BlankGenerator.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/utils/irange.h"
#include "yuri/core/utils/assign_events.h"
#include <functional>
#include <iostream>
namespace yuri {

namespace blank {

MODULE_REGISTRATION_BEGIN("blank")
		REGISTER_IOTHREAD("blank", BlankGenerator)
MODULE_REGISTRATION_END()


IOTHREAD_GENERATOR(BlankGenerator)


core::Parameters BlankGenerator::configure()
{
	auto p = IOThread::configure();
	p["format"]["Format (RGB, YUV422, ...)"]="YUV";
	p["fps"]["Framerate"]=25;
	p["resolution"]["Resolution of the image"]=resolution_t{640,480};
	p["color"]["Frame color"]=core::color_t::create_rgb(0,0,0);
	return p;
}

BlankGenerator::BlankGenerator(log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
IOThread(log_, parent, 0, 1, "BlankGenerator"),
BasicEventConsumer(log),
fps_(25),resolution_{640,480},format_(core::raw_format::yuyv422),
color_(core::color_t::create_rgb(0,0,0))
{
	set_latency(10_ms);
	IOTHREAD_INIT(parameters)

}

namespace {


// Using multiple specializations to prevent compiler warnings(gcc-4.9)

template<typename T, int s>
typename std::enable_if<(s > 0), T>::type
shift(T val) {
	return val<<s;
}

template<typename T, int s>
typename std::enable_if<(s < 0), T>::type
shift(T val) {
	return val>>(-s);
}

template<typename T, int s>
typename std::enable_if<s == 0, T>::type
shift(T val) {
	return val;
}


template<typename T, int s, typename T2>
typename std::enable_if<!std::is_same<T, T2>::value, T>::type
shift(T2 val) {
	return shift<T, s>(static_cast<T>(val));
}


template<typename T, typename T2, T2 mask1, int sh1, T2 mask2, int sh2, T2 mask3, int sh3>
T get_low_bit_color3(T2 v1, T2 v2, T2 v3)
{
	return  shift<T, sh1>(v1 & mask1) |
			shift<T, sh2>(v2 & mask2) |
			shift<T, sh3>(v3 & mask3);
}

template<typename T, typename T2, T2 mask1, int sh1, T2 mask2, int sh2, T2 mask3, int sh3, T2 mask4, int sh4>
T get_low_bit_color4(T2 v1, T2 v2, T2 v3, T2 v4)
{
	return  shift<T, sh1>(v1 & mask1) |
			shift<T, sh2>(v2 & mask2) |
			shift<T, sh3>(v3 & mask3) |
			shift<T, sh4>(v4 & mask4);
}

template<typename T, typename T2, T2 mask1, int sh1, T2 mask2, int sh2, T2 mask3, int sh3>
T get_low_bit_color3(const std::array<T2, 3>& v)
{
	return  get_low_bit_color3<T, T2, mask1, sh1, mask2, sh2, mask3, sh3>(v[0], v[1], v[2]);
}

template<typename T, typename T2, T2 mask1, int sh1, T2 mask2, int sh2, T2 mask3, int sh3, T2 mask4, int sh4>
T get_low_bit_color4(const std::array<T2, 4>& v)
{
	return  get_low_bit_color4<T, T2, mask1, sh1, mask2, sh2, mask3, sh3, mask4, sh4>(v[0], v[1], v[2], v[3]);
}

template<typename T>
void fill_plane(void* begin0, size_t lines, size_t linesize_bytes, const T& color)
{
	const auto linesize = linesize_bytes / sizeof(T);
	auto begin = reinterpret_cast<T*>(begin0);
	for (auto line: irange(lines)) {
		(void)line;
		std::fill(begin, begin+linesize, color);
		begin+=linesize;
	}
}


template<size_t dim, typename T = uint8_t>
void fill_plane(void* begin0, size_t lines, size_t linesize_bytes, const std::array<T, dim>& colors)
{
	const auto linesize = linesize_bytes / sizeof(T);
	auto begin = reinterpret_cast<T*>(begin0);
	auto start = begin;
	const auto end_line = begin + linesize;
	while(start <= (end_line - dim) ) {
		std::copy(colors.begin(), colors.end(), start);
		start += dim;
	}
	for (auto line: irange(1, lines)) {
		std::copy(begin, end_line, begin + line * linesize);
	}
}

template<size_t dim, typename T = uint8_t>
void fill_frame_plane(const core::pRawVideoFrame& frame, size_t plane, const std::array<T, dim>& colors)
{
	fill_plane<dim, T>(PLANE_RAW_DATA(frame, plane), frame->get_resolution().height, PLANE_DATA(frame, 0).get_line_size(), colors);
}


template<typename T = uint8_t>
void fill_frame_plane(const core::pRawVideoFrame& frame, size_t plane, const T& colors)
{
	fill_plane<T>(PLANE_RAW_DATA(frame, plane), PLANE_DATA(frame, plane).get_resolution().height, PLANE_DATA(frame, plane).get_line_size(), colors);
}


/*!
 * This is purely an optimization. It won't work on big endian systems ...
 * @param frame
 * @param plane
 * @param colors
 */
template<>
void fill_frame_plane<4, uint8_t>(const core::pRawVideoFrame& frame, size_t plane, const std::array<uint8_t, 4>& colors)
{
	// This will work only on little endian systems....
	auto col = get_low_bit_color4<uint32_t, uint8_t, 0xFF, 0, 0xFF, 8, 0xFF, 16, 0xFF, 24>(colors);
	fill_frame_plane<uint32_t>(frame, plane, col);
}
template<>
void fill_frame_plane<4, uint16_t>(const core::pRawVideoFrame& frame, size_t plane, const std::array<uint16_t, 4>& colors)
{
	// This will work only on little endian systems....
	auto col = get_low_bit_color4<uint64_t, uint16_t, 0xFFFF, 0, 0xFFFF, 16, 0xFFFF, 32, 0xFFFF, 48>(colors);
	fill_frame_plane<uint64_t>(frame, plane, col);
}

}

core::pRawVideoFrame BlankGenerator::generate_frame(format_t format, resolution_t resolution, core::color_t color)
{
	auto frame = core::RawVideoFrame::create_empty(format, resolution);
	using namespace core::raw_format;

	switch (format) {
		case rgb24:
			fill_frame_plane<3>(frame, 0, color.get_rgb());
			break;
		case bgr24:
			fill_frame_plane<3>(frame, 0, {{color.b(), color.g(), color.r()}});
			break;
		case rgba32:
			fill_frame_plane<4>(frame, 0, color.get_rgba());
			break;
		case bgra32:
			fill_frame_plane<4>(frame, 0, {{color.b(), color.g(), color.r(), color.a()}});
			break;
		case argb32:
			fill_frame_plane<4>(frame, 0, {{color.a(), color.r(), color.g(), color.b()}});
			break;
		case abgr32:
			fill_frame_plane<4>(frame, 0, {{color.a(), color.b(), color.g(), color.r()}});
			break;

		case rgb48:
			fill_frame_plane<3, uint16_t>(frame, 0, color.get_rgb16());
			break;
		case bgr48:
			fill_frame_plane<3, uint16_t>(frame, 0, {{color.b16(), color.g16(), color.r16()}});
			break;
		case rgba64:
			fill_frame_plane<4, uint16_t>(frame, 0, color.get_rgba16());
			break;
		case bgra64:
			fill_frame_plane<4, uint16_t>(frame, 0, {{color.b16(), color.g16(), color.r16(), color.a16()}});
			break;
		case argb64:
			fill_frame_plane<4, uint16_t>(frame, 0, {{color.a16(), color.r16(), color.g16(), color.b16()}});
			break;
		case abgr64:
			fill_frame_plane<4, uint16_t>(frame, 0, {{color.a16(), color.b16(), color.g16(), color.r16()}});
			break;

		case rgb24p:
			fill_frame_plane(frame, 0, color.r());
			fill_frame_plane(frame, 1, color.g());
			fill_frame_plane(frame, 2, color.b());
			break;
		case bgr24p:
			fill_frame_plane(frame, 0, color.b());
			fill_frame_plane(frame, 1, color.g());
			fill_frame_plane(frame, 2, color.r());
			break;
		case gbr24p:
			fill_frame_plane(frame, 0, color.g());
			fill_frame_plane(frame, 1, color.b());
			fill_frame_plane(frame, 2, color.r());
			break;
		case rgba32p:
			fill_frame_plane(frame, 0, color.r());
			fill_frame_plane(frame, 1, color.g());
			fill_frame_plane(frame, 2, color.b());
			fill_frame_plane(frame, 3, color.a());
			break;
		case abgr32p:
			fill_frame_plane(frame, 0, color.a());
			fill_frame_plane(frame, 1, color.b());
			fill_frame_plane(frame, 2, color.g());
			fill_frame_plane(frame, 3, color.r());

			break;

		case rgb16:
			fill_frame_plane<uint16_t>(frame, 0, get_low_bit_color3<uint16_t, uint8_t, 0xF8, 8, 0xFC, 3, 0xF8, -3>(color.get_rgb()));
			break;
		case bgr16:
			fill_frame_plane<uint16_t>(frame, 0, get_low_bit_color3<uint16_t, uint8_t, 0xF8, -3, 0xFC, 3, 0xF8, 8>(color.get_rgb()));
			break;
		case rgb15:
			fill_frame_plane<uint16_t>(frame, 0, get_low_bit_color3<uint16_t, uint8_t, 0xF8, 7, 0xF8, 2, 0xF8, -3>(color.get_rgb()));
			break;
		case bgr15:
			fill_frame_plane<uint16_t>(frame, 0, get_low_bit_color3<uint16_t, uint8_t, 0xF8, -3, 0xF8, 2, 0xF8, -2>(color.get_rgb()));
			break;
		case rgb8:
			fill_frame_plane<uint8_t>(frame, 0, get_low_bit_color3<uint8_t, uint8_t, 0xE0, 0, 0xE0, -3, 0xC0, -6>(color.get_rgb()));
			break;
		case bgr8:
			fill_frame_plane<uint8_t>(frame, 0, get_low_bit_color3<uint8_t, uint8_t, 0xE0, -5, 0xE0, -2, 0xC0, 0>(color.get_rgb()));
			break;


		case yuv444:
			fill_frame_plane<3>(frame, 0, color.get_yuv());
			break;
		case yuva4444:
			fill_frame_plane<4>(frame, 0, color.get_yuva());
			break;
		case ayuv4444:
			fill_frame_plane<4>(frame, 0, {{color.a(), color.y(), color.u(), color.v()}});
			break;
		case yuyv422:
			fill_frame_plane<4>(frame, 0, {{color.y(), color.u(), color.y(), color.v()}});
			break;
		case yvyu422:
			fill_frame_plane<4>(frame, 0, {{color.y(), color.v(), color.y(), color.u()}});
			break;
		case uyvy422:
			fill_frame_plane<4>(frame, 0, {{color.u(), color.y(), color.v(), color.y()}});
			break;
		case vyuy422:
			fill_frame_plane<4>(frame, 0, {{color.v(), color.y(), color.u(), color.y()}});
			break;

		case yuv411:
			fill_frame_plane<6>(frame, 0, {{color.y(), color.y(), color.u(), color.y(), color.y(), color.v()}});
			break;
		case yvu411:
			fill_frame_plane<6>(frame, 0, {{color.y(), color.y(), color.v(), color.y(), color.y(), color.u()}});
			break;
		case yuv444p:
		case yuv422p:
		case yuv420p:
		case yuv411p:
			fill_frame_plane(frame, 0, color.y());
			fill_frame_plane(frame, 1, color.u());
			fill_frame_plane(frame, 2, color.v());
			break;
		case y8:
			fill_frame_plane(frame, 0, color.y());
			break;
		case y16:
			fill_frame_plane(frame, 0, color.y16());
			break;
		default:
			log[log::warning] << "Generated frame in format " << core::raw_format::get_format_name(format) << ", but don't know how to initialize it!";
			break;

	}

	return frame;
}



void BlankGenerator::run()
{
	next_time_ = timestamp_t{};
	while(still_running()) {
		process_events();
		const auto curtime = timestamp_t{};
		if (next_time_ > curtime) {
			sleep(std::min(get_latency(), next_time_ - curtime));
			continue;
		}

		if (!frame_cache_ && resolution_) {
			Timer t0;
			frame_cache_ = generate_frame(format_, resolution_, color_);
			log[log::debug] << "Generated frame in " << t0.get_duration();
		}


		if (frame_cache_) {
			push_frame(0, frame_cache_);
		}
		next_time_ = next_time_ + (1_s / fps_);
	}
}

bool BlankGenerator::set_param(const core::Parameter &param)
{
	if (assign_parameters(param)
		(fps_, 			"fps")
		(resolution_, 	"resolution")
		.parsed<std::string>
			(format_, 	"format", core::raw_format::parse_format)
		(color_, 		"color"))
		return true;

	return IOThread::set_param(param);
}

bool BlankGenerator::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
	if (assign_events(event_name, event)
			(resolution_, 	"resolution")
			.parsed<std::string>
				(format_, 	"format", core::raw_format::parse_format)
			(color_, 		"color")) {
		frame_cache_.reset();
		return true;
	}
	if (assign_events(event_name, event)
			(fps_, 			"fps")) {
		return true;
	}
	return false;
}

}

}
