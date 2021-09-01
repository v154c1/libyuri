//
// Created by neneko on 01.09.21.
//

#include "convert_common.h"

namespace yuri {
    namespace video {

        template<>
        void convert_line<core::raw_format::u8, core::raw_format::y8>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            std::copy(src, src+width, dest);
        }

        template<>
        void convert_line<core::raw_format::v8, core::raw_format::y8>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            std::copy(src, src+width, dest);
        }
        template<>
        void convert_line<core::raw_format::r8, core::raw_format::y8>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            std::copy(src, src+width, dest);
        }
        template<>
        void convert_line<core::raw_format::g8, core::raw_format::y8>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            std::copy(src, src+width, dest);
        }
        template<>
        void convert_line<core::raw_format::b8, core::raw_format::y8>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            std::copy(src, src+width, dest);
        }
        template<>
        void convert_line<core::raw_format::depth8, core::raw_format::y8>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            std::copy(src, src+width, dest);
        }

        template<>
        void convert_line<core::raw_format::y8, core::raw_format::y16>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + width;
            while (src < end) {
                *dest++ = 0;
                *dest++ = *src++;
            }
        }

        template<>
        void convert_line<core::raw_format::y16, core::raw_format::y8>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + 2 * width;
            src++;
            while (src < end) {
                *dest++ = *src;
                src+=2;
            }
        }

        template<>
        void convert_line<core::raw_format::yuyv422, core::raw_format::y8>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + 2 * width;
            while (src < end) {
                *dest++ = *src;
                src+=2;
            }
        }

        template<>
        void convert_line<core::raw_format::yuyv422, core::raw_format::y16>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + 2 * width;
            while (src < end) {
                *dest++ = 0;
                *dest++ = *src;
                src+=2;
            }
        }

        template<>
        void convert_line<core::raw_format::yvyu422, core::raw_format::y8>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            return convert_line<core::raw_format::yuyv422, core::raw_format::y8>(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::yvyu422, core::raw_format::y16>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            return convert_line<core::raw_format::yuyv422, core::raw_format::y16>(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::uyvy422, core::raw_format::y8>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + 2 * width;
            ++src;
            while (src < end) {
                *dest++ = *src;
                src+=2;
            }
        }

        template<>
        void convert_line<core::raw_format::uyvy422, core::raw_format::y16>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + 2 * width;
            ++src;
            while (src < end) {
                *dest++ = 0;
                *dest++ = *src;
                src+=2;
            }
        }

        template<>
        void convert_line<core::raw_format::vyuy422, core::raw_format::y8>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            return convert_line<core::raw_format::uyvy422, core::raw_format::y8>(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::vyuy422, core::raw_format::y16>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            return convert_line<core::raw_format::uyvy422, core::raw_format::y16>(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::y8, core::raw_format::yuyv422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + width;
            while (src < end) {
                *dest++ = *src++;
                *dest++ = 128;
            }
        }

        template<>
        void convert_line<core::raw_format::y8, core::raw_format::yvyu422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            return convert_line<core::raw_format::y8, core::raw_format::yuyv422>(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::y8, core::raw_format::uyvy422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + width;
            while (src < end) {
                *dest++ = 128;
                *dest++ = *src++;
            }
        }

        template<>
        void convert_line<core::raw_format::y8, core::raw_format::vyuy422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            return convert_line<core::raw_format::y8, core::raw_format::uyvy422>(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::y8, core::raw_format::yuv444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + width;
            while (src < end) {
                *dest++ = *src++;
                *dest++ = 128;
                *dest++ = 128;
            }
        }

        template<>
        void convert_line<core::raw_format::y8, core::raw_format::yuv411>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + width;
            while (src < end) {
                *dest++ = *src++;
                *dest++ = *src++;
                *dest++ = 128;
            }
        }

        template<>
        void convert_line<core::raw_format::y16, core::raw_format::yuyv422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + width * 2;
            ++src;
            while (src < end) {
                *dest++ = *src;
                *dest++ = 128;
                src+=2;
            }
        }

        template<>
        void convert_line<core::raw_format::y16, core::raw_format::yvyu422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            return convert_line<core::raw_format::y8, core::raw_format::yuyv422>(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::y16, core::raw_format::uyvy422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + width * 2;
            ++src;
            while (src < end) {
                *dest++ = 128;
                *dest++ = *src;
                src+=2;
            }
        }

        template<>
        void convert_line<core::raw_format::y16, core::raw_format::vyuy422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            return convert_line<core::raw_format::y8, core::raw_format::uyvy422>(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::y16, core::raw_format::yuv444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + width * 2;
            ++src;
            while (src < end) {
                *dest++ = *src;
                *dest++ = 128;
                *dest++ = 128;
                src+=2;
            }
        }

        template<>
        void convert_line<core::raw_format::y16, core::raw_format::yuv411>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + width * 2 - 2*(width%2);
            ++src;
            while (src < end) {
                *dest++ = *src;
                src+=2;
                *dest++ = *src;
                src+=2;
                *dest++ = 128;
            }
        }


        template<>
        void convert_line<core::raw_format::y8, core::raw_format::rgb24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + width;
            while (src < end) {
                *dest++ = *src;
                *dest++ = *src;
                *dest++ = *src++;
            }
        }

        template<>
        void convert_line<core::raw_format::rgb24, core::raw_format::y8>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
        {
            const auto end = src + width;
            while (src < end) {
                *dest++ = static_cast<uint8_t>(
                        (static_cast<uint_fast16_t>(*(src+0)) +
                         static_cast<uint_fast16_t>(*(src+1)) +
                         static_cast<uint_fast16_t>(*(src+2))) / 3);
                src+=3;
            }
        }
        converter_map get_converters_single() {
            static std::map<format_pair_t, std::pair<converter_t, size_t>> converters_single = {
                    ADD_CONVERSION(core::raw_format::u8, core::raw_format::y8, 1)
                    ADD_CONVERSION(core::raw_format::v8, core::raw_format::y8, 1)
                    ADD_CONVERSION(core::raw_format::r8, core::raw_format::y8, 1)
                    ADD_CONVERSION(core::raw_format::g8, core::raw_format::y8, 1)
                    ADD_CONVERSION(core::raw_format::b8, core::raw_format::y8, 1)
                    ADD_CONVERSION(core::raw_format::depth8, core::raw_format::y8, 1)
                    ADD_CONVERSION(core::raw_format::y8, core::raw_format::y16, 10)
                    ADD_CONVERSION(core::raw_format::y16, core::raw_format::y8, 20)
                    ADD_CONVERSION(core::raw_format::yuyv422, core::raw_format::y8, 40)
                    ADD_CONVERSION(core::raw_format::yuyv422, core::raw_format::y16, 40)
                    ADD_CONVERSION(core::raw_format::yvyu422, core::raw_format::y8, 40)
                    ADD_CONVERSION(core::raw_format::yvyu422, core::raw_format::y16, 40)
                    ADD_CONVERSION(core::raw_format::uyvy422, core::raw_format::y8, 40)
                    ADD_CONVERSION(core::raw_format::uyvy422, core::raw_format::y16, 40)
                    ADD_CONVERSION(core::raw_format::vyuy422, core::raw_format::y8, 40)
                    ADD_CONVERSION(core::raw_format::vyuy422, core::raw_format::y16, 40)
                    ADD_CONVERSION(core::raw_format::y8, core::raw_format::yuyv422, 20)
                    ADD_CONVERSION(core::raw_format::y8, core::raw_format::yvyu422, 20)
                    ADD_CONVERSION(core::raw_format::y8, core::raw_format::uyvy422, 20)
                    ADD_CONVERSION(core::raw_format::y8, core::raw_format::vyuy422, 20)
                    ADD_CONVERSION(core::raw_format::y8, core::raw_format::yuv411, 20)
                    ADD_CONVERSION(core::raw_format::y8, core::raw_format::yuv444, 20)
                    ADD_CONVERSION(core::raw_format::y16, core::raw_format::yuyv422, 30)
                    ADD_CONVERSION(core::raw_format::y16, core::raw_format::yvyu422, 30)
                    ADD_CONVERSION(core::raw_format::y16, core::raw_format::uyvy422, 30)
                    ADD_CONVERSION(core::raw_format::y16, core::raw_format::vyuy422, 30)
                    ADD_CONVERSION(core::raw_format::y16, core::raw_format::yuv411, 30)
                    ADD_CONVERSION(core::raw_format::y16, core::raw_format::yuv444, 30)
                    ADD_CONVERSION(core::raw_format::y8, core::raw_format::rgb24, 10)
                    ADD_CONVERSION(core::raw_format::rgb24, core::raw_format::y8, 50)
                    ADD_CONVERSION(core::raw_format::rgb48, core::raw_format::rgb24, 30)
                    ADD_CONVERSION(core::raw_format::bgr48, core::raw_format::bgr24, 30)
                    ADD_CONVERSION(core::raw_format::rgb48, core::raw_format::bgr24, 30)
                    ADD_CONVERSION(core::raw_format::bgr48, core::raw_format::rgb24, 30)
                    ADD_CONVERSION(core::raw_format::rgb24, core::raw_format::rgb16, 10)
                    ADD_CONVERSION(core::raw_format::rgb24, core::raw_format::bgr16, 10)
            };
            return converters_single;
        }
    }
    }