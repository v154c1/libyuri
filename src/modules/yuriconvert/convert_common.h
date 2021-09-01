//
// Created by neneko on 01.09.21.
//

#ifndef YURI2_CONVERT_COMMON_H
#define YURI2_CONVERT_COMMON_H
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include <future>
#include "yuri/core/utils/irange.h"

namespace yuri {
    namespace video {
        class YuriConvertor;

        using converter_t = std::function<core::pRawVideoFrame(const core::pRawVideoFrame &, const YuriConvertor &,
                                                               size_t)>;
        using format_pair_t = std::pair<yuri::format_t, yuri::format_t>;

        using converter_map = std::map<format_pair_t, std::pair<converter_t, size_t>>;

//	inline unsigned int convY(unsigned int Y) { return (Y*219+4128) >> 6; }
//	inline unsigned int convC(unsigned int C) {	return (C*7+129) >> 1; }

        template<format_t fmt>
        core::pRawVideoFrame allocate_frame(size_t width, size_t height);

//	template<format_t fmt>
//		size_t get_linesize(size_t width);
        template<format_t fmt_in, format_t fmt_out>
        void
        convert_line(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor &);

        template<format_t fmt_in, format_t fmt_out>
        void convert_line(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width);

        template<format_t fmt_in, format_t fmt_out>
        core::pRawVideoFrame convert_formats(const core::pRawVideoFrame &frame, const YuriConvertor &, size_t threads);

#define ADD_CONVERSION(fmt1, fmt2, cost) {std::make_pair(fmt1, fmt2), std::make_pair(&convert_formats<fmt1, fmt2>, cost)},



        /* ***************************************************************************
 * 					Default template definitions
 *************************************************************************** */

        template<format_t fmt_in, format_t fmt_out>
        void convert_line(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor&)
        {
            return convert_line<fmt_in, fmt_out>(src, dest, width);
        }

        template<format_t fmt>
        core::pRawVideoFrame allocate_frame(size_t width, size_t height)
        {
            return core::RawVideoFrame::create_empty(fmt, {width, height}, true);
        }
//template<format_t fmt> size_t get_linesize(size_t width)
//{
//	//FormatInfo_t fi_in			= core::BasicPipe::get_format_info(fmt);
//	//assert (fi_in && !(fi_in->bpp % 8));
//	//return (width * fi_in->bpp) / 8;
//	const auto& fi = core::raw_format::get_format_info(fmt);
//	assert (fi.planes.size()==1);
//	assert (fi.planes[0].bit_depth.first % (8* fi.planes[0].bit_depth.second)== 0);
//	return width * fi.planes[0].bit_depth.first / (8* fi.planes[0].bit_depth.second);
//}

        template<format_t fmt_in, format_t fmt_out>
        void convert_multiple_lines(const size_t linesize_in,
                                    const size_t linesize_out,
                                    core::Plane::const_iterator src,
                                    core::Plane::iterator dest,
                                    const size_t width,
                                    const YuriConvertor& conv,
                                    size_t lines)
        {
            for (size_t line = 0; line < lines; ++line) {
                convert_line<fmt_in, fmt_out>(src, dest, width, conv);
                src+=linesize_in;
                dest+=linesize_out;
            }
        }

        template<format_t fmt_in, format_t fmt_out>
        core::pRawVideoFrame convert_formats(const core::pRawVideoFrame& frame, const YuriConvertor& conv, size_t threads)
        {
            const size_t width 			= frame->get_width();
            const size_t height			= frame->get_height();
            core::pRawVideoFrame outframe 	= allocate_frame<fmt_out>(width, height);
            outframe->copy_video_params(*frame);
//	const size_t linesize_in 	= get_linesize<fmt_in>(width);
//	const size_t linesize_out 	= get_linesize<fmt_out>(width);
            const size_t linesize_in 	= PLANE_DATA(frame,0).get_line_size();
            const size_t linesize_out 	= PLANE_DATA(outframe,0).get_line_size();
            core::Plane::const_iterator src	= PLANE_DATA(frame,0).begin();
            core::Plane::iterator dest		= PLANE_DATA(outframe,0).begin();

            if (!threads || threads == 1) {
                for (size_t line = 0; line < height; ++line) {
                    convert_line<fmt_in, fmt_out>(src, dest, width, conv);
                    src+=linesize_in;
                    dest+=linesize_out;
                }
            } else {
                size_t task_lines = height / threads;
                std::vector<std::future<void>> results;
                size_t start_line = 0;
                size_t remaining = height;
                auto f = [&](size_t s, size_t r) {
                    return convert_multiple_lines<fmt_in, fmt_out>(
                            linesize_in,
                            linesize_out,
                            src + s * linesize_in,
                            dest + s * linesize_out,
                            width,
                            conv,
                            std::min(task_lines, r)
                    );
                };
                for (auto t: irange(threads)) {
                    (void)t;
                    results.push_back(
                            std::async(std::launch::async,
                                       f, start_line, remaining)
                    );
                    start_line += task_lines;
                    remaining -= task_lines;
                }
                for (auto& t: results) {
                    t.get();
                }
            }
            return outframe;
        }




    }
}

#endif //YURI2_CONVERT_COMMON_H
