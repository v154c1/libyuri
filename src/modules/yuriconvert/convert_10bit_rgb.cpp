//
// Created by neneko on 01.09.21.
//

#include "convert_common.h"

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

                uint32_t extend_8bit(uint8_t value) {
                    return static_cast<uint32_t>(value) << 2;
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

                inline void store_components(core::Plane::iterator dest, uint32_t c0, uint32_t c1, uint32_t c2) {
                    *dest++ = ((c2 << 2) & 0xFC);
                    *dest++ = ((c1 << 4) & 0xF0) | ((c2 >> 6) & 0x0F);
                    *dest++ = ((c0 << 6) & 0xC0) | ((c1 >> 4) & 0x3F);
                    *dest++ = (c0 >> 2) & 0xFF;
                }

                inline void store_components_8bit(core::Plane::iterator dest, uint8_t c0, uint8_t c1, uint8_t c2) {
                    store_components(dest, r10k::extend_8bit(c0), r10k::extend_8bit(c1), r10k::extend_8bit(c2));
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
        void convert_line<core::raw_format::rgb_r10k_le, core::raw_format::bgr24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                // Skipping lowest 2 bits for every component
                *dest++ = r10k_le::r10k_component_2(src) >> 2;
                *dest++ = r10k_le::r10k_component_1(src) >> 2;
                *dest++ = r10k_le::r10k_component_0(src) >> 2;
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
        void convert_line<core::raw_format::rgb_r10k_le, core::raw_format::bgr48>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                // Skipping lowest 2 bits for every component
                *dest++ = r10k_le::r10k_component_2(src) << 6;
                *dest++ = r10k_le::r10k_component_1(src) << 6;
                *dest++ = r10k_le::r10k_component_0(src) << 6;
                src += 4;
            }
        }

        template<>
        void convert_line<core::raw_format::rgb24, core::raw_format::rgb_r10k_le>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {

                r10k_le::store_components_8bit(dest, src[0], src[1], src[2]);
                dest += 4;
                src += 3;
            }
        }

        template<>
        void convert_line<core::raw_format::bgr24, core::raw_format::rgb_r10k_le>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {

                r10k_le::store_components_8bit(dest, src[2], src[1], src[0]);
                dest += 4;
                src += 3;
            }
        }

        converter_map get_converters_rgb10bit() {
            static std::map<format_pair_t, std::pair<converter_t, size_t>> converters_rgb10bit = {
                    ADD_CONVERSION(core::raw_format::rgb_r10k_be, core::raw_format::rgb24, 30)
                    ADD_CONVERSION(core::raw_format::rgb_r10k_be, core::raw_format::bgr24, 30)
                    ADD_CONVERSION(core::raw_format::rgb_r10k_be, core::raw_format::rgba32, 30)
                    ADD_CONVERSION(core::raw_format::rgb_r10k_be, core::raw_format::bgra32, 30)
                    ADD_CONVERSION(core::raw_format::rgb_r10k_le, core::raw_format::rgb24, 30)
                    ADD_CONVERSION(core::raw_format::rgb_r10k_le, core::raw_format::bgr24, 30)
                    ADD_CONVERSION(core::raw_format::rgb_r10k_le, core::raw_format::rgb48, 30)
                    ADD_CONVERSION(core::raw_format::rgb_r10k_le, core::raw_format::bgr48, 30)
                    ADD_CONVERSION(core::raw_format::rgb24, core::raw_format::rgb_r10k_le, 50)
                    ADD_CONVERSION(core::raw_format::bgr24, core::raw_format::rgb_r10k_le, 50)
            };
            return converters_rgb10bit;
        }

    }
}