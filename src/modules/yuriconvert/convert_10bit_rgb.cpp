//
// Created by neneko on 01.09.21.
//

#include "convert_10bit_rgb.h"

namespace yuri {
    namespace video {
//        namespace rgb10bit {
        namespace {
            namespace r10k {
                // get (src0 & m0) << off0  | (src1 & m1 ) >> off1
                template<size_t m0, size_t off0, size_t m1, size_t off1>
                inline uint16_t
                shifted_component(core::Plane::const_reference src0, core::Plane::const_reference src1) {
//            return static_cast<uint32_t>(1023 * (
//                    (
//                        ((static_cast<uint32_t>(src0) & m0) << off0 ) |
//                        ((static_cast<uint32_t>(src1) & m1) >> off1 )
//                    ) - 64) / 896);
                    return ((static_cast<uint16_t>(src0) & m0) << off0) |
                           ((static_cast<uint16_t>(src1) & m1) >> off1);

                }
            }
            namespace r10k_be {
                inline uint16_t r10k_component_0(core::Plane::const_iterator src) {
                    return r10k::shifted_component<0x3F, 4, 0xF0, 4>(src[0], src[1]);// >> 2;
                    //return ((src[0] * 0x3F) << 4) | ((src[1] & 0xF0) >> 4);
                }

                inline uint16_t r10k_component_1(core::Plane::const_iterator src) {
                    return r10k::shifted_component<0x0F, 6, 0xFC, 2>(src[1], src[2]);// >> 2;
//            return ((src[1] * 0x0F) << 6) | ((src[2] & 0xFC) >> 2);
                }

                inline uint16_t r10k_component_2(core::Plane::const_iterator src) {
                    return r10k::shifted_component<0x03, 8, 0xFF, 0>(src[2], src[3]);// >> 2;
//            return ((src[2] * 0x03) << 8) | ((src[3] & 0xFF) >> 0);
                }
            }
            namespace r10k_le {
                inline uint16_t r10k_component_0(core::Plane::const_iterator src) {
                    return r10k::shifted_component<0xFF, 2, 0xC0, 6>(src[3], src[2]);// >> 2;
//            return ((src[3] * 0x3F) << 4) | ((src[2] & 0xF0) >> 4);
                }

                inline uint16_t r10k_component_1(core::Plane::const_iterator src) {
                    return r10k::shifted_component<0x3F, 4, 0xF0, 4>(src[2], src[1]);// >> 2;
//            return ((src[2] * 0x0F) << 6) | ((src[1] & 0xFC) >> 2);
                }

                inline uint16_t r10k_component_2(core::Plane::const_iterator src) {
                    return r10k::shifted_component<0x0F, 6, 0xFC, 2>(src[1], src[0]);// >> 2;
//            return ((src[1] * 0x03) << 8) | ((src[0] & 0xFF) >> 0);
                }
            }
        }

        template<>
        void convert_line<core::raw_format::rgb_r10k_be, core::raw_format::rgb24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                // Skipping lowest 2 bits for every component
                *dest++ = r10k_be::r10k_component_0(src) >> 2;
                *dest++ = r10k_be::r10k_component_1(src) >> 2;
                *dest++ = r10k_be::r10k_component_2(src) >> 2;
                src += 4;
            }
        }

        template<>
        void convert_line<core::raw_format::rgb_r10k_be, core::raw_format::bgr24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                // Skipping lowest 2 bits for every component
                *dest++ = r10k_be::r10k_component_2(src) >> 2;
                *dest++ = r10k_be::r10k_component_1(src) >> 2;
                *dest++ = r10k_be::r10k_component_0(src) >> 2;
                src += 4;
            }
        }

        template<>
        void convert_line<core::raw_format::rgb_r10k_be, core::raw_format::rgba32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                // Skipping lowest 2 bits for every component
                *dest++ = r10k_be::r10k_component_0(src) >> 2;
                *dest++ = r10k_be::r10k_component_1(src) >> 2;
                *dest++ = r10k_be::r10k_component_2(src) >> 2;
                *dest++ = 255;
                src += 4;
            }
        }

        template<>
        void convert_line<core::raw_format::rgb_r10k_be, core::raw_format::bgra32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                // Skipping lowest 2 bits for every component
                *dest++ = r10k_be::r10k_component_2(src) >> 2;
                *dest++ = r10k_be::r10k_component_1(src) >> 2;
                *dest++ = r10k_be::r10k_component_0(src) >> 2;
                *dest++ = 255;
                src += 4;
            }
        }

        template<>
        void convert_line<core::raw_format::rgb_r10k_le, core::raw_format::rgb24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                // Skipping lowest 2 bits for every component
                *dest++ = r10k_le::r10k_component_0(src) >> 2;
                *dest++ = r10k_le::r10k_component_1(src) >> 2;
                *dest++ = r10k_le::r10k_component_2(src) >> 2;
                src += 4;
            }
        }

        template<>
        void convert_line<core::raw_format::rgb_r10k_le, core::raw_format::rgb48>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                // Skipping lowest 2 bits for every component
                *dest++ = r10k_le::r10k_component_0(src) << 6;
                *dest++ = r10k_le::r10k_component_1(src) << 6;
                *dest++ = r10k_le::r10k_component_2(src) << 6;
                src += 4;
            }
        }

        template<>
        void convert_line<core::raw_format::rgb24, core::raw_format::rgb_r10k_le>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                const auto r = static_cast<uint32_t>(src[0]) << 2;//* 896 /256 + 64;
                const auto g = static_cast<uint32_t>(src[1]) << 2;//* 896 /256 + 64 ;
                const auto b = static_cast<uint32_t>(src[2]) << 2;//* 896 /256 + 64;
//
//        *dest++=0x00;
//
//        *dest++=0x00;
//
//        *dest++=0xC0;
//
//        *dest++=0xFF;

//        *dest++ = ((b  << 0) & 0xFF );
//        *dest++ = ((g  << 2) & 0xFC ) | ((b >> 8) & 0x03);
//        *dest++ = ((r  << 4) & 0xF0 ) | ((g >> 6) & 0x0F);
//        *dest++ = (r >> 4)  &0x3F;

                *dest++ = ((b << 2) & 0xFC);
                *dest++ = ((g << 4) & 0xF0) | ((b >> 6) & 0x0F);
                *dest++ = ((r << 6) & 0xC0) | ((g >> 4) & 0x3F);
                *dest++ = (r >> 2) & 0xFF;


                src += 3;
            }
//            }

        }

        converter_map get_converters_rgb10bit() {
            static std::map<format_pair_t, std::pair<converter_t, size_t>> converters_rgb10bit = {
                    ADD_CONVERSION(core::raw_format::rgb_r10k_be, core::raw_format::rgb24, 30)
                    ADD_CONVERSION(core::raw_format::rgb_r10k_be, core::raw_format::bgr24, 30)
                    ADD_CONVERSION(core::raw_format::rgb_r10k_be, core::raw_format::rgba32, 30)
                    ADD_CONVERSION(core::raw_format::rgb_r10k_be, core::raw_format::bgra32, 30)
                    ADD_CONVERSION(core::raw_format::rgb_r10k_le, core::raw_format::rgb24, 30)
                    ADD_CONVERSION(core::raw_format::rgb_r10k_le, core::raw_format::rgb48, 30)
                    ADD_CONVERSION(core::raw_format::rgb24, core::raw_format::rgb_r10k_le, 50)
            };
            return converters_rgb10bit;
        }

    }
}