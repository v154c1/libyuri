//
// Created by neneko on 01.09.21.
//

/* ***************************************************************************
 * 					RGB conversions
 *************************************************************************** */
#include "convert_rgb.h"

namespace yuri {
    namespace video {


        void rgb_rgba(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, uint8_t alpha) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = *src++;
                *dest++ = *src++;
                *dest++ = *src++;
                *dest++ = alpha;
            }
        }

        template<>
        void convert_line<core::raw_format::rgb24, core::raw_format::rgba32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgb_rgba(src, dest, width, 255);
        }

        template<>
        void convert_line<core::raw_format::bgr24, core::raw_format::bgra32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgb_rgba(src, dest, width, 255);
        }

        void rgb_argb(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, uint8_t alpha) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = alpha;
                *dest++ = *src++;
                *dest++ = *src++;
                *dest++ = *src++;
            }
        }

        template<>
        void convert_line<core::raw_format::rgb24, core::raw_format::argb32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgb_argb(src, dest, width, 255);
        }

        template<>
        void convert_line<core::raw_format::bgr24, core::raw_format::abgr32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgb_argb(src, dest, width, 255);
        }

        void rgba_rgb(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = *src++;
                *dest++ = *src++;
                *dest++ = *src++;
                src++;
            }
        }

        template<>
        void convert_line<core::raw_format::rgba32, core::raw_format::rgb24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgba_rgb(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::bgra32, core::raw_format::bgr24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgba_rgb(src, dest, width);
        }

        void argb_rgb(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                src++;
                *dest++ = *src++;
                *dest++ = *src++;
                *dest++ = *src++;
            }
        }

        template<>
        void convert_line<core::raw_format::argb32, core::raw_format::rgb24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            argb_rgb(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::abgr32, core::raw_format::bgr24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            argb_rgb(src, dest, width);
        }

        void rgba_bgra(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = *(src + 2);
                *dest++ = *(src + 1);
                *dest++ = *(src + 0);
                *dest++ = *(src + 3);
                src += 4;
            }
        }

        template<>
        void convert_line<core::raw_format::bgra32, core::raw_format::rgba32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgba_bgra(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::rgba32, core::raw_format::bgra32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgba_bgra(src, dest, width);
        }

        void argb_abgr(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = *(src + 0);
                *dest++ = *(src + 3);
                *dest++ = *(src + 2);
                *dest++ = *(src + 1);
                src += 4;
            }
        }

        template<>
        void convert_line<core::raw_format::abgr32, core::raw_format::argb32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            argb_abgr(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::argb32, core::raw_format::abgr32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            argb_abgr(src, dest, width);
        }

        void rgba_abgr(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = *(src + 3);
                *dest++ = *(src + 2);
                *dest++ = *(src + 1);
                *dest++ = *(src + 0);
                src += 4;
            }
        }

        template<>
        void convert_line<core::raw_format::bgra32, core::raw_format::argb32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
//	rgba_bgra(src, dest, width);
            rgba_abgr(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::argb32, core::raw_format::bgra32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
//	rgba_bgra(src, dest, width);
            rgba_abgr(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::abgr32, core::raw_format::rgba32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
//	rgba_bgra(src, dest, width);
            rgba_abgr(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::rgba32, core::raw_format::abgr32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
//	rgba_bgra(src, dest, width);
            rgba_abgr(src, dest, width);
        }

        void rgba_argb(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = *(src + 3);
                *dest++ = *(src + 0);
                *dest++ = *(src + 1);
                *dest++ = *(src + 2);
                src += 4;
            }
        }

        template<>
        void convert_line<core::raw_format::rgba32, core::raw_format::argb32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgba_argb(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::bgra32, core::raw_format::abgr32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgba_argb(src, dest, width);
        }

        void argb_rgba(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = *(src + 1);
                *dest++ = *(src + 2);
                *dest++ = *(src + 3);
                *dest++ = *(src + 0);
                src += 4;
            }
        }

        template<>
        void convert_line<core::raw_format::argb32, core::raw_format::rgba32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            argb_rgba(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::abgr32, core::raw_format::bgra32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            argb_rgba(src, dest, width);
        }


        void rgb_bgra(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, uint8_t alpha) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = *(src + 2);
                *dest++ = *(src + 1);
                *dest++ = *(src + 0);
                *dest++ = alpha;
                src += 3;
            }
        }

        template<>
        void convert_line<core::raw_format::rgb24, core::raw_format::bgra32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgb_bgra(src, dest, width, 255);
        }

        template<>
        void convert_line<core::raw_format::bgr24, core::raw_format::rgba32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgb_bgra(src, dest, width, 255);
        }

        void rgb_abgr(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, uint8_t alpha) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = alpha;
                *dest++ = *(src + 2);
                *dest++ = *(src + 1);
                *dest++ = *(src + 0);
                src += 3;
            }
        }

        template<>
        void convert_line<core::raw_format::rgb24, core::raw_format::abgr32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgb_abgr(src, dest, width, 255);
        }

        template<>
        void convert_line<core::raw_format::bgr24, core::raw_format::argb32>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgb_abgr(src, dest, width, 255);
        }

        void bgra_rgb(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = *(src + 2);
                *dest++ = *(src + 1);
                *dest++ = *(src + 0);
                src += 4;
            }
        }

        template<>
        void convert_line<core::raw_format::bgra32, core::raw_format::rgb24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            bgra_rgb(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::rgba32, core::raw_format::bgr24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            bgra_rgb(src, dest, width);
        }

        void abgr_rgb(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = *(src + 3);
                *dest++ = *(src + 2);
                *dest++ = *(src + 1);
                src += 4;
            }
        }

        template<>
        void convert_line<core::raw_format::abgr32, core::raw_format::rgb24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            abgr_rgb(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::argb32, core::raw_format::bgr24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            abgr_rgb(src, dest, width);
        }

        void bgr_rgb(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = *(src + 2);
                *dest++ = *(src + 1);
                *dest++ = *(src + 0);
                src += 3;
            }
        }

        template<>
        void convert_line<core::raw_format::bgr24, core::raw_format::rgb24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            bgr_rgb(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::rgb24, core::raw_format::bgr24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            bgr_rgb(src, dest, width);
        }

        void gbr_rgb(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = *(src + 2);
                *dest++ = *(src + 0);
                *dest++ = *(src + 1);
                src += 3;
            }
        }

        template<>
        void convert_line<core::raw_format::gbr24, core::raw_format::rgb24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            gbr_rgb(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::rgb24, core::raw_format::gbr24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            gbr_rgb(src, dest, width);
        }

// LE 16bit rgb -> 8bit rgb
        void rgb48_rgb(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = *(src + 1);
                *dest++ = *(src + 3);
                *dest++ = *(src + 5);
                src += 6;
            }
        }

        template<>
        void convert_line<core::raw_format::rgb48, core::raw_format::rgb24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgb48_rgb(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::bgr48, core::raw_format::bgr24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgb48_rgb(src, dest, width);
        }

// LE 16bit rgb -> 8bit bgr
        void rgb48_bgr(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                *dest++ = *(src + 5);
                *dest++ = *(src + 3);
                *dest++ = *(src + 1);
                src += 6;
            }
        }

        template<>
        void convert_line<core::raw_format::rgb48, core::raw_format::bgr24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgb48_bgr(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::bgr48, core::raw_format::rgb24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            rgb48_bgr(src, dest, width);
        }

        template<>
        void convert_line<core::raw_format::rgb24, core::raw_format::rgb16>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                const uint16_t col =
                        ((src[0] >> 3) << 11) |
                        ((src[1] >> 2) << 5) |
                        ((src[2] >> 3) << 0);
                *dest++ = col & 0xFF;
                *dest++ = col >> 8;
                src += 3;
            }
        }

        template<>
        void convert_line<core::raw_format::rgb24, core::raw_format::bgr16>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width) {
            for (size_t pixel = 0; pixel < width; ++pixel) {
                const uint16_t col =
                        ((src[2] >> 3) << 11) |
                        ((src[1] >> 2) << 5) |
                        ((src[0] >> 3) << 0);
                *dest++ = col & 0xFF;
                *dest++ = col >> 8;
                src += 3;
            }
        }

        converter_map get_converters_rgb() {
            static std::map<format_pair_t, std::pair<converter_t, size_t>> converters_rgb = {
                    ADD_CONVERSION(core::raw_format::rgb24, core::raw_format::rgba32, 12)
                    ADD_CONVERSION(core::raw_format::bgr24, core::raw_format::bgra32, 12)
                    ADD_CONVERSION(core::raw_format::rgb24, core::raw_format::argb32, 12)
                    ADD_CONVERSION(core::raw_format::bgr24, core::raw_format::abgr32, 12)
                    ADD_CONVERSION(core::raw_format::rgba32, core::raw_format::rgb24, 12)
                    ADD_CONVERSION(core::raw_format::bgra32, core::raw_format::bgr24, 12)
                    ADD_CONVERSION(core::raw_format::argb32, core::raw_format::rgb24, 12)
                    ADD_CONVERSION(core::raw_format::abgr32, core::raw_format::bgr24, 12)
                    ADD_CONVERSION(core::raw_format::rgba32, core::raw_format::argb32, 10)
                    ADD_CONVERSION(core::raw_format::rgba32, core::raw_format::abgr32, 10)
                    ADD_CONVERSION(core::raw_format::rgba32, core::raw_format::bgra32, 10)
                    ADD_CONVERSION(core::raw_format::argb32, core::raw_format::rgba32, 10)
                    ADD_CONVERSION(core::raw_format::argb32, core::raw_format::abgr32, 10)
                    ADD_CONVERSION(core::raw_format::argb32, core::raw_format::bgra32, 10)
                    ADD_CONVERSION(core::raw_format::bgra32, core::raw_format::rgba32, 10)
                    ADD_CONVERSION(core::raw_format::bgra32, core::raw_format::argb32, 10)
                    ADD_CONVERSION(core::raw_format::bgra32, core::raw_format::abgr32, 10)
                    ADD_CONVERSION(core::raw_format::abgr32, core::raw_format::argb32, 10)
                    ADD_CONVERSION(core::raw_format::abgr32, core::raw_format::rgba32, 10)
                    ADD_CONVERSION(core::raw_format::abgr32, core::raw_format::bgra32, 10)
                    ADD_CONVERSION(core::raw_format::rgb24, core::raw_format::bgra32, 12)
                    ADD_CONVERSION(core::raw_format::bgr24, core::raw_format::rgba32, 12)
                    ADD_CONVERSION(core::raw_format::rgb24, core::raw_format::abgr32, 12)
                    ADD_CONVERSION(core::raw_format::bgr24, core::raw_format::argb32, 12)
                    ADD_CONVERSION(core::raw_format::rgba32, core::raw_format::bgr24, 12)
                    ADD_CONVERSION(core::raw_format::bgra32, core::raw_format::rgb24, 12)
                    ADD_CONVERSION(core::raw_format::argb32, core::raw_format::bgr24, 12)
                    ADD_CONVERSION(core::raw_format::abgr32, core::raw_format::rgb24, 12)
                    ADD_CONVERSION(core::raw_format::bgr24, core::raw_format::rgb24, 10)
                    ADD_CONVERSION(core::raw_format::rgb24, core::raw_format::bgr24, 10)
                    ADD_CONVERSION(core::raw_format::gbr24, core::raw_format::rgb24, 10)
                    ADD_CONVERSION(core::raw_format::rgb24, core::raw_format::gbr24, 10)
            };
            return converters_rgb;
        }

    }
}
