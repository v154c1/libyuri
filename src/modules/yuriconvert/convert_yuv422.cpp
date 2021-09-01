//
// Created by neneko on 01.09.21.
//

#include "convert_yuv422.h"

namespace yuri {
    namespace video {


// Converts abcd -> badc
        inline void swap_yuv422_pairs
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++=*(src+1);
                *dest++=*src;
                src+=2;
            }
        }
// Converts abcd -> cbad
        inline void swap_yuv422_0_2
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            for (size_t pixel = 0; pixel < width/2; ++pixel) {
                *dest++=*(src+2);
                *dest++=*(src+1);
                *dest++=*(src+0);
                *dest++=*(src+3);
                src+=4;
            }
        }
// Converts abcd -> adcb
        inline void swap_yuv422_1_3
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            for (size_t pixel = 0; pixel < width/2; ++pixel) {
                *dest++=*(src+0);
                *dest++=*(src+3);
                *dest++=*(src+2);
                *dest++=*(src+1);
                src+=4;
            }
        }
// Converts abcd -> bcda
        inline void swap_yuv422_pairs_1_3
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            for (size_t pixel = 0; pixel < width/2; ++pixel) {
                *dest++=*(src+1);
                *dest++=*(src+2);
                *dest++=*(src+3);
                *dest++=*(src+0);
                src+=4;
            }
        }
// Converts abcd -> dabc
        inline void swap_yuv422_pairs_0_2
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            for (size_t pixel = 0; pixel < width/2; ++pixel) {
                *dest++=*(src+3);
                *dest++=*(src+0);
                *dest++=*(src+1);
                *dest++=*(src+2);
                src+=4;
            }
        }
        template<>
        void convert_line<core::raw_format::yuyv422, core::raw_format::uyvy422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            swap_yuv422_pairs(src,dest,width);
        }
        template<>
        void convert_line<core::raw_format::uyvy422, core::raw_format::yuyv422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            swap_yuv422_pairs(src,dest,width);
        }
        template<>
        void convert_line<core::raw_format::vyuy422, core::raw_format::yvyu422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            swap_yuv422_pairs(src,dest,width);
        }
        template<>
        void convert_line<core::raw_format::yvyu422, core::raw_format::vyuy422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            swap_yuv422_pairs(src,dest,width);
        }

        template<>
        void convert_line<core::raw_format::uyvy422, core::raw_format::vyuy422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            swap_yuv422_0_2(src,dest,width);
        }
        template<>
        void convert_line<core::raw_format::vyuy422, core::raw_format::uyvy422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            swap_yuv422_0_2(src,dest,width);
        }
        template<>
        void convert_line<core::raw_format::yuyv422, core::raw_format::yvyu422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            swap_yuv422_1_3(src,dest,width);
        }
        template<>
        void convert_line<core::raw_format::yvyu422, core::raw_format::yuyv422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            swap_yuv422_1_3(src,dest,width);
        }
        template<>
        void convert_line<core::raw_format::uyvy422, core::raw_format::yvyu422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            swap_yuv422_pairs_1_3(src,dest,width);
        }
        template<>
        void convert_line<core::raw_format::vyuy422, core::raw_format::yuyv422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            swap_yuv422_pairs_1_3(src,dest,width);
        }
        template<>
        void convert_line<core::raw_format::yuyv422, core::raw_format::vyuy422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            swap_yuv422_pairs_0_2(src,dest,width);
        }
        template<>
        void convert_line<core::raw_format::yvyu422, core::raw_format::uyvy422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            swap_yuv422_pairs_0_2(src,dest,width);
        }


        converter_map get_converters_yuv422() {
            static std::map<format_pair_t, std::pair<converter_t, size_t>> converters_yuv422 = {
                    ADD_CONVERSION(core::raw_format::yuyv422, core::raw_format::uyvy422, 10)
                    ADD_CONVERSION(core::raw_format::uyvy422, core::raw_format::yuyv422, 10)
                    ADD_CONVERSION(core::raw_format::yvyu422, core::raw_format::vyuy422, 10)
                    ADD_CONVERSION(core::raw_format::vyuy422, core::raw_format::yvyu422, 10)
                    ADD_CONVERSION(core::raw_format::uyvy422, core::raw_format::vyuy422, 10)
                    ADD_CONVERSION(core::raw_format::vyuy422, core::raw_format::uyvy422, 10)
                    ADD_CONVERSION(core::raw_format::yuyv422, core::raw_format::yvyu422, 10)
                    ADD_CONVERSION(core::raw_format::yvyu422, core::raw_format::yuyv422, 10)
                    ADD_CONVERSION(core::raw_format::uyvy422, core::raw_format::yvyu422, 10)
                    ADD_CONVERSION(core::raw_format::vyuy422, core::raw_format::yuyv422, 10)
                    ADD_CONVERSION(core::raw_format::yuyv422, core::raw_format::vyuy422, 10)
                    ADD_CONVERSION(core::raw_format::yvyu422, core::raw_format::uyvy422, 10)
            };
            return converters_yuv422;
        }
    }

}