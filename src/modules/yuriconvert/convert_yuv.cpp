//
// Created by neneko on 01.09.21.
//

#include "convert_common.h"

namespace yuri {
    namespace video {

        template<>
        void convert_line<core::raw_format::yuyv422, core::raw_format::yuv444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            uint8_t y, u, v;
            for (size_t pixel = 0; pixel < width / 2; ++pixel) {
                *dest++ = *src++;
                u = *dest++ = *src++;
                y = *src++;
                v = *src++;
                *dest++ = v;
                *dest++ = y;
                *dest++ = u;
                *dest++ = v;
            }
        }

        template<>
        void convert_line<core::raw_format::yuv444, core::raw_format::yuyv422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width / 2; ++pixel) {
                *dest++ = *src++;
                *dest++ = *src++;
                src++;
                *dest++ = *src++;
                src++;
                *dest++ = *src++;
            }
        }

        template<>
        void convert_line<core::raw_format::uyvy422, core::raw_format::yuv444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            uint8_t y, u, v;
            for (size_t pixel = 0; pixel < width / 2; ++pixel) {
                u = *src++;
                *dest++ = *src++;
                *dest++ = u;
                v = *src++;
                y = *src++;
                *dest++ = v;
                *dest++ = y;
                *dest++ = u;
                *dest++ = v;
            }
        }

        template<>
        void convert_line<core::raw_format::yuv444, core::raw_format::uyvy422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            uint8_t y;
            for (size_t pixel = 0; pixel < width / 2; ++pixel) {
                y = *src++;
                *dest++ = *src++;
                *dest++ = y;
                src++;
                y = *src++;
                src++;
                *dest++ = *src++;
                *dest++ = y;
            }
        }

        template<>
        void convert_line<core::raw_format::yuva4444, core::raw_format::yuv444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = *src++;
                *dest++ = *src++;
                *dest++ = *src++;
                src++;
            }
        }

        template<>
        void convert_line<core::raw_format::ayuv4444, core::raw_format::yuv444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                src++;
                *dest++ = *src++;
                *dest++ = *src++;
                *dest++ = *src++;
            }
        }

        template<>
        void convert_line<core::raw_format::yuv444, core::raw_format::yuva4444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = *src++;
                *dest++ = *src++;
                *dest++ = *src++;
                *dest++ = 255;
            }
        }

        template<>
        void convert_line<core::raw_format::yuv444, core::raw_format::ayuv4444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = 255;
                *dest++ = *src++;
                *dest++ = *src++;
                *dest++ = *src++;
            }
        }

        template<>
        void convert_line<core::raw_format::yuva4444, core::raw_format::yuyv422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width / 2; ++pixel) {
                *dest++ = *src++;
                *dest++ = *src++;
                src++;
                src++;
                *dest++ = *src++;
                src++;
                *dest++ = *src++;
                src++;
            }
        }

        template<>
        void convert_line<core::raw_format::ayuv4444, core::raw_format::yuyv422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width / 2; ++pixel) {
                src++;
                *dest++ = *src++;
                *dest++ = *src++;

                src++;
                src++;
                *dest++ = *src++;
                src++;
                *dest++ = *src++;
            }
        }



        template<>
        void convert_line<core::raw_format::yuv444, core::raw_format::yuv411>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + 3 * width - (width % 4);
            while (src < end) {

                auto u = static_cast<uint8_t>((static_cast<uint_fast16_t>(*(src+1)) + *(src+4)  + *(src+7) + *(src+10)) >> 2);
                auto v = static_cast<uint8_t>((static_cast<uint_fast16_t>(*(src+2)) + *(src+5)  + *(src+8) + *(src+11)) >> 2);
                *dest++ = *(src+0);
                *dest++ = *(src+3);
                *dest++ = u;
                *dest++ = *(src+6);
                *dest++ = *(src+9);
                *dest++ = v;
                src+=12;
            }
        }

        template<>
        void convert_line<core::raw_format::yuv444, core::raw_format::yvu411>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + 3 * width - (width % 4);
            while (src < end) {

                auto u = static_cast<uint8_t>((static_cast<uint_fast16_t>(*(src+1)) + *(src+4)  + *(src+7) + *(src+10)) >> 2);
                auto v = static_cast<uint8_t>((static_cast<uint_fast16_t>(*(src+2)) + *(src+5)  + *(src+8) + *(src+11)) >> 2);
                *dest++ = *(src+0);
                *dest++ = *(src+3);
                *dest++ = v;
                *dest++ = *(src+6);
                *dest++ = *(src+9);
                *dest++ = u;
                src+=12;
            }
        }

        template<>
        void convert_line<core::raw_format::yuv411, core::raw_format::yuyv422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            for (size_t pix = 0; pix < width; pix+=4) {
                *dest++ = *src++;
                const auto y1 = *src++;
                const auto u = *src++;
                const auto y2 = *src++;
                const auto y3 = *src++;
                const auto v = *src++;
                *dest++ = u;
                *dest++ = y1;
                *dest++ = v;
                *dest++ = y2;
                *dest++ = u;
                *dest++ = y3;
                *dest++ = v;
            }
        }

        template<>
        void convert_line<core::raw_format::yuyv422, core::raw_format::yuv411>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + 2 * width - (width % 4);
            while (src < end) {
                auto u = static_cast<uint8_t>((static_cast<uint_fast16_t>(*(src+1)) + *(src+5)) >> 1);
                auto v = static_cast<uint8_t>((static_cast<uint_fast16_t>(*(src+3)) + *(src+7)) >> 1);
                *dest++ = *(src+0);
                *dest++ = *(src+2);
                *dest++ = u;
                *dest++ = *(src+4);
                *dest++ = *(src+6);
                *dest++ = v;
                src+=8;
            }
        }


        template<>
        void convert_line<core::raw_format::yvyu422, core::raw_format::yuv411>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + 2 * width - (width % 4);
            while (src < end) {
                auto u = static_cast<uint8_t>((static_cast<uint_fast16_t>(*(src+3)) + *(src+7)) >> 1);
                auto v = static_cast<uint8_t>((static_cast<uint_fast16_t>(*(src+1)) + *(src+5)) >> 1);
                *dest++ = *(src+0);
                *dest++ = *(src+2);
                *dest++ = u;
                *dest++ = *(src+4);
                *dest++ = *(src+6);
                *dest++ = v;
                src+=8;
            }
        }

        template<>
        void convert_line<core::raw_format::uyvy422, core::raw_format::yuv411>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + 2 * width - (width % 4);
            while (src < end) {
                auto u = static_cast<uint8_t>((static_cast<uint_fast16_t>(*(src+0)) + *(src+4)) >> 1);
                auto v = static_cast<uint8_t>((static_cast<uint_fast16_t>(*(src+2)) + *(src+6)) >> 1);
                *dest++ = *(src+1);
                *dest++ = *(src+3);
                *dest++ = u;
                *dest++ = *(src+5);
                *dest++ = *(src+7);
                *dest++ = v;
                src+=8;
            }
        }

        template<>
        void convert_line<core::raw_format::vyuy422, core::raw_format::yuv411>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + 2 * width - (width % 4);
            while (src < end) {
                auto u = static_cast<uint8_t>((static_cast<uint_fast16_t>(*(src+2)) + *(src+6)) >> 1);
                auto v = static_cast<uint8_t>((static_cast<uint_fast16_t>(*(src+0)) + *(src+4)) >> 1);
                *dest++ = *(src+1);
                *dest++ = *(src+3);
                *dest++ = u;
                *dest++ = *(src+5);
                *dest++ = *(src+7);
                *dest++ = v;
                src+=8;
            }
        }

        template<>
        void convert_line<core::raw_format::yuyv422, core::raw_format::yvu411>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            return convert_line<core::raw_format::yvyu422, core::raw_format::yuv411>(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::yvyu422, core::raw_format::yvu411>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            return convert_line<core::raw_format::yuyv422, core::raw_format::yuv411>(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::uyvy422, core::raw_format::yvu411>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            return convert_line<core::raw_format::vyuy422, core::raw_format::yuv411>(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::vyuy422, core::raw_format::yvu411>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            return convert_line<core::raw_format::uyvy422, core::raw_format::yuv411>(src, dest, width);
        }


        std::map<format_pair_t, std::pair<converter_t, size_t>> get_converters_yuv() {
            return std::map<format_pair_t, std::pair<converter_t, size_t>>{
                    ADD_CONVERSION(core::raw_format::yuyv422, core::raw_format::yuv444, 15)
                    ADD_CONVERSION(core::raw_format::yuv444, core::raw_format::yuyv422, 15)
                    ADD_CONVERSION(core::raw_format::uyvy422, core::raw_format::yuv444, 15)
                    ADD_CONVERSION(core::raw_format::yuv444, core::raw_format::uyvy422, 15)
                    ADD_CONVERSION(core::raw_format::yuva4444, core::raw_format::yuv444, 10)
                    ADD_CONVERSION(core::raw_format::ayuv4444, core::raw_format::yuv444, 10)
                    ADD_CONVERSION(core::raw_format::yuv444, core::raw_format::yuva4444, 10)
                    ADD_CONVERSION(core::raw_format::yuv444, core::raw_format::ayuv4444, 10)
                    ADD_CONVERSION(core::raw_format::yuva4444, core::raw_format::yuyv422, 15)
                    ADD_CONVERSION(core::raw_format::ayuv4444, core::raw_format::yuyv422, 15)
                    ADD_CONVERSION(core::raw_format::yuv444, core::raw_format::yuv411, 20)
                    ADD_CONVERSION(core::raw_format::yuv444, core::raw_format::yvu411, 20)
                    ADD_CONVERSION(core::raw_format::yuv411, core::raw_format::yuyv422, 10)
                    ADD_CONVERSION(core::raw_format::yuyv422, core::raw_format::yuv411, 15)
                    ADD_CONVERSION(core::raw_format::yvyu422, core::raw_format::yuv411, 15)
                    ADD_CONVERSION(core::raw_format::uyvy422, core::raw_format::yuv411, 15)
                    ADD_CONVERSION(core::raw_format::vyuy422, core::raw_format::yuv411, 15)
                    ADD_CONVERSION(core::raw_format::yuyv422, core::raw_format::yvu411, 15)
                    ADD_CONVERSION(core::raw_format::yvyu422, core::raw_format::yvu411, 15)
                    ADD_CONVERSION(core::raw_format::uyvy422, core::raw_format::yvu411, 15)
                    ADD_CONVERSION(core::raw_format::vyuy422, core::raw_format::yvu411, 15)
            };
        }
    }

}