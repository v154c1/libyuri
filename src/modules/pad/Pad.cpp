/*!
 * @file 		Pad.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		14.08.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "Pad.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/frame/raw_frame_types.h"
#include <array>
namespace yuri {
namespace pad {


IOTHREAD_GENERATOR(Pad)
MODULE_REGISTRATION_BEGIN("pad")
		REGISTER_IOTHREAD("pad",Pad)
MODULE_REGISTRATION_END()

core::Parameters Pad::configure()
{
	core::Parameters p = core::IOThread::configure();
	p.set_description("Adds letterbox around the image to fill specified dimensions. Image width should be multiply of 4");
//	p->set_max_pipes(1,1);
//	p["width"]["Width of the destination image"] = 800;
//	p["height"]["Height of the destination image"] = 600;
	p["resolution"]["Resolution of the destination image"]=resolution_t{800,600};
	p["halign"]["Horizontal alignment of the image inside the canvas. (center, left, right)"]=std::string("center");
	p["valign"]["Vertical alignment of the image inside the canvas. (center, top, bottom)"]=std::string("center");
	p["color"]["background color"]=core::color_t::create_rgb(0,0,0);
	return p;
}

namespace {
std::vector<format_t> query_supported_formats()
{
	std::vector<format_t> fmts;
	for (const auto f: core::raw_format::formats()) {
		const auto& info = f.second;
		if (info.planes.size() != 1) {
			continue;
		}
		if (info.planes[0].bit_depth.first % (8 * info.planes[0].bit_depth.second)) {
			continue;
		}
		fmts.push_back(f.first);
	}
	return fmts;
}

}
Pad::Pad(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::SpecializedIOFilter<core::RawVideoFrame>(log_,parent,std::string("pad")),
resolution_(resolution_t{800, 600}), halign_(horizontal_alignment_t::center),
valign_(vertical_alignment_t::center),color_(core::color_t::create_rgb(0,0,0))
{
	IOTHREAD_INIT(parameters)
	set_supported_formats(query_supported_formats());
}

Pad::~Pad() noexcept
{
}

namespace {
size_t count_empty_lines_top(size_t height_in, size_t height_out, vertical_alignment_t valign)
{
	if (valign == vertical_alignment_t::top) return 0;
	if (valign == vertical_alignment_t::bottom) return height_out>height_in?height_out-height_in:0;
	return height_out>height_in?(height_out-height_in)/2:0;
}
size_t count_empty_lines_bottom(size_t height_in, size_t height_out, size_t skip_start, vertical_alignment_t valign)
{
	if (valign == vertical_alignment_t::bottom) return 0;
	if (valign == vertical_alignment_t::top) return height_out>height_in?height_out-height_in:0;
	return height_out>height_in?height_out-height_in-skip_start:0;
}
size_t count_empty_cols_left(size_t width_in, size_t width_out, horizontal_alignment_t halign)
{
	if (halign == horizontal_alignment_t::left) return 0;
	if (halign == horizontal_alignment_t::right) return width_out>width_in?width_out-width_in:0;
	return width_out>width_in?(width_out-width_in)/2:0;
}
size_t count_empty_cols_right(size_t width_in, size_t width_out, size_t skip_left, horizontal_alignment_t halign)
{
	if (halign == horizontal_alignment_t::right) return 0;
	if (halign == horizontal_alignment_t::left) return width_out>width_in?width_out-width_in:0;
	return width_out>width_in?width_out-width_in-skip_left:0;
}

template<size_t N, class Iter, class value_type = typename std::iterator_traits<Iter>::value_type>
void fill_pattern(Iter start, const Iter& end, const std::array<value_type, N>& pattern)
{
	const size_t pattern_size = pattern.size();
//	assert(pattern_size > 0);
	const auto pat_start = pattern.begin();
	const auto pat_end = pattern.end();

	size_t remaining = std::distance(start, end);

	while (remaining > pattern_size) {
		std::copy(pat_start, pat_end, start);
		std::advance(start, pattern_size);
		remaining-=pattern_size;
	}
	std::copy(pat_start, pat_start+remaining, start);
}
/*!
 * Function to fill in black pixels - either 0s or specialized pattern
 * @param start		Iterator to beginning of the range
 * @param end		Iterator to the end of the range
 * @param format	format of the pixels
 */
template<class Iter>
void fill_color(Iter start, const Iter& end, format_t format, const core::color_t& color)
{
	using namespace core;
	using T = typename std::remove_reference<decltype(*start)>::type;
	switch (format) {
		case raw_format::yuv444:
			fill_pattern<3>(start, end, color.get_yuv());
			break;
		case raw_format::yuyv422:
			fill_pattern<4>(start, end, std::array<T, 4>{{color.y(), color.u(), color.y(), color.v()}});
			break;
		case raw_format::yvyu422:
			fill_pattern<4>(start, end, std::array<T, 4>{{color.y(), color.v(), color.y(), color.u()}});
			break;
		case raw_format::uyvy422:
			fill_pattern<4>(start, end, std::array<T, 4>{{color.u(), color.y(), color.v(), color.y()}});
			break;
		case raw_format::vyuy422:
			fill_pattern<4>(start, end, std::array<T, 4>{{color.v(), color.y(), color.u(), color.y()}});
			break;
		case raw_format::rgb24:
			fill_pattern<3>(start, end, color.get_rgb());
			break;
		case raw_format::rgba32:
			fill_pattern<4>(start, end, color.get_rgba());
			break;
		case raw_format::bgr24:
			fill_pattern<3>(start, end, std::array<T, 3>{{color.b(), color.g(), color.r()}});
			break;
		case raw_format::abgr32:
			fill_pattern<4>(start, end, std::array<T, 4>{{color.a(), color.b(), color.g(), color.r()}});
			break;
		case raw_format::bgra32:
			fill_pattern<4>(start, end, std::array<T, 4>{{color.b(), color.g(), color.r(), color.a()}});
			break;
		case raw_format::argb32:
			fill_pattern<4>(start, end, std::array<T, 4>{{color.a(), color.r(), color.g(), color.b()}});
			break;
		case raw_format::y8:
			std::fill(start,end,color.y());
			break;
		default:std::fill(start,end,0);break;

	}
}

/*!
 * Copies black samples from to a destination.
 *
 * The function assumes @em sample is an iterator pointing to at least @em end - @em start valid sample bytes
 * @param start
 * @param end
 * @param sample
 */
template<class Iter, class Iter2>
void fill_from_sample(Iter start, const Iter& end, const Iter2& sample)
{
	std::copy(sample, sample + std::distance(start, end), start);
}

}


core::pFrame Pad::do_special_single_step(core::pRawVideoFrame frame)
{
	if (resolution_ == resolution_t{0,0}) return frame;
//	const core::pRawVideoFrame frame= 	dynamic_pointer_cast<core::RawVideoFrame>(frame_in);
	const resolution_t resolution	= frame->get_resolution();
	if (resolution == resolution_) return frame;
	const yuri::size_t height_in	= resolution.height;
	const yuri::size_t width_in		= resolution.width;
	const format_t format 			= frame->get_format();
	//const FormatInfo_t info 		= core::BasicPipe::get_format_info(format);
	const auto& info				= core::raw_format::get_format_info(format);
	const yuri::size_t height_out	= resolution_.height;
	const yuri::size_t width_out	= resolution_.width;

	if (info.planes.size() != 1) {
		log[log::warning] << "Input frames has to have only single image plane";
		return core::pFrame{};
	}
	if (info.planes[0].bit_depth.first % (8 * info.planes[0].bit_depth.second)) {
		log[log::warning] << "Input frames has to have bit depth divisible by 8";
		return core::pFrame{};
	}

	const yuri::size_t Bpp 			= info.planes[0].bit_depth.first / (8 * info.planes[0].bit_depth.second);

	const size_t line_size_in		= width_in  * Bpp;
	const size_t line_size_out		= width_out * Bpp;

	const size_t blank_lines_top 	= count_empty_lines_top(height_in, height_out, valign_);
	const size_t skip_lines_top 	= count_empty_lines_top(height_out, height_in, valign_);
	const size_t blank_lines_bottom = count_empty_lines_bottom(height_in, height_out, blank_lines_top, valign_);
	//const size_t skip_lines_bottom 	= count_empty_lines_bottom(height_, height, skip_lines_top, valign_);

	const size_t blank_cols_left 	= count_empty_cols_left(width_in, width_out, halign_);
	const size_t skip_cols_left 	= count_empty_cols_left(width_out, width_in, halign_);
	const size_t blank_cols_right 	= count_empty_cols_right(width_in, width_out, blank_cols_left, halign_);
	//const size_t skip_cols_right 	= count_empty_cols_right(width_, width, skip_cols_left, halign_);

	const size_t copy_width			= width_out - blank_cols_left - blank_cols_right;
	const size_t copy_size			= copy_width * Bpp;

	log[log::verbose_debug] << "Padding with " << blank_cols_left << " pixels left, " << blank_cols_right << "pixels right, "
				<< blank_lines_top << " pixels on the top and " << blank_lines_bottom << " pixels on the bottom";

	//core::pBasicFrame output = allocate_empty_frame(format, width_, height_);
	core::pRawVideoFrame output		= core::RawVideoFrame::create_empty(format, resolution_, true);

	const auto data_in_start		= PLANE_DATA(frame,0).begin()+skip_lines_top*line_size_in+skip_cols_left*Bpp;
//	const auto data_in_end			= PLANE_DATA(frame,0).end();
	const auto data_out_start		= PLANE_DATA(output,0).begin();

	// One line of pre-prepared black samples to speed up the filling up process later.
	uvector<uint8_t> samples_black(line_size_out);
	fill_color(samples_black.begin(), samples_black.end(), format, color_);


	// Fill in empty lines at the top
	for (size_t line = 0; line < blank_lines_top; ++line) {
		fill_from_sample(data_out_start+line*line_size_out, data_out_start+(line+1)*line_size_out, samples_black.begin());
	}
	// Fill in empty lines at the bottom
	for (size_t line = height_out - blank_lines_bottom; line < height_out; ++line) {
		fill_from_sample(data_out_start+line*line_size_out, data_out_start+(line+1)*line_size_out, samples_black.begin());
	}
	auto data_in = data_in_start;
	// Process all non-empty lines
	for (size_t line = blank_lines_top; line < height_out - blank_lines_bottom; ++line) {
		const auto out_line_start			= data_out_start + line_size_out * line ;
		const auto next_line_start 			= out_line_start + line_size_out;
		const auto out_line_active_start 	= out_line_start + blank_cols_left * Bpp;
		const auto out_line_active_end 		= out_line_active_start + copy_size;
		// Fill in blank pixels at left side
		fill_from_sample(out_line_start, out_line_active_start, samples_black.begin());

		// Copy pixels from input
		std::copy(data_in, data_in + copy_size, out_line_active_start);
		std::advance(data_in, line_size_in);

		// Fill in blank pixels at left side
		fill_from_sample(out_line_active_end, next_line_start, samples_black.begin());
	}
	output->copy_video_params(*frame);
	return output;
}

namespace {
std::map<std::string, horizontal_alignment_t> halign_strings {
	{"left", horizontal_alignment_t::left},
	{"center", horizontal_alignment_t::center},
	{"right", horizontal_alignment_t::right}
};
std::map<std::string, vertical_alignment_t> valign_strings {
	{"top", vertical_alignment_t::top},
	{"center", vertical_alignment_t::center},
	{"bottom", vertical_alignment_t::bottom},
};

horizontal_alignment_t parse_halign(const std::string& align)
{
	auto it=halign_strings.find(align);
	if (it == halign_strings.end()) return horizontal_alignment_t::center;
	return it->second;
}
vertical_alignment_t parse_valign(const std::string& align)
{
	auto it=valign_strings.find(align);
	if (it == valign_strings.end()) return vertical_alignment_t::center;
	return it->second;
}
}
bool Pad::set_param(const core::Parameter& param)
{
	if (assign_parameters(param)
			(resolution_, "resolution")
			.parsed<std::string>
				(halign_, "halign", parse_halign)
			.parsed<std::string>
				(valign_, "valign", parse_valign)
			(color_, "color"))
		return true;
	return core::IOThread::set_param(param);
}

} /* namespace pad */
} /* namespace yuri */
