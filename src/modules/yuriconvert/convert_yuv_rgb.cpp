//
// Created by neneko on 01.09.21.
//

#include "convert_yuv_rgb.h"
#include "YuriConvert.h"

namespace yuri{
    namespace  video {

        namespace {
            const size_t Wr_601 = 2990;
            const size_t Wb_601 = 1140;
            const size_t Wr_709 = 2126;
            const size_t Wb_709 = 722;
            const size_t Wr_2020 = 2627;
            const size_t Wb_2020 = 593;
        }

        template<size_t wr, size_t wb>
        struct colorimetry_traits {
            static inline constexpr double Wr() { return static_cast<double>(wr)/10000.0; }
            static inline constexpr double Wb() { return static_cast<double>(wb)/10000.0; }
            static inline constexpr double Wg() { return 1.0 - Wr() - Wb(); }
            static inline constexpr double Kb() { return 0.5 / (1.0 - Wb()); }
            static inline constexpr double Kr() { return 0.5 / (1.0 - Wr()); }
            static inline constexpr double WbKbWg() { return  Wb()/Kb()/Wg(); }
            static inline constexpr double WrKrWg() { return  Wr()/Kr()/Wg(); }
        };

        template<template <class, bool> class func, class colorimetry>
        void convert_rgb_yuv_dispatch(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, bool full_range)
        {
            if (full_range) func<colorimetry, true>::eval(src, dest, width);
            else func<colorimetry, false>::eval(src, dest, width);
        }
        template<template <class, bool> class func>
        void convert_rgb_yuv_dispatch(core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, colorimetry_t col, bool full_range)
        {
            switch (col) {
                case YURI_COLORIMETRY_REC601:
                    convert_rgb_yuv_dispatch<func, colorimetry_traits<Wr_601, Wb_601> >(src, dest, width, full_range);
                    break;
                case YURI_COLORIMETRY_REC2020:
                    convert_rgb_yuv_dispatch<func, colorimetry_traits<Wr_2020, Wb_2020> >(src, dest, width, full_range);
                    break;
                case YURI_COLORIMETRY_REC709:
                default:
                    convert_rgb_yuv_dispatch<func, colorimetry_traits<Wr_709, Wb_709> >(src, dest, width, full_range);
                    break;
            }
        }

        template<bool full_range>
        uint8_t convert_y_from_double(double value);

        template<>
        uint8_t convert_y_from_double<true>(double value)
        {
            return static_cast<uint8_t>(value*255.0);
        }
        template<>
        uint8_t convert_y_from_double<false>(double value)
        {
            return static_cast<uint8_t>(value*219.0)+16;
        }

        template<bool full_range>
        uint8_t convert_c_from_double(double value);

        template<>
        uint8_t convert_c_from_double<true>(double value)
        {
            return static_cast<uint8_t>(255.0 * std::min(std::max((value+0.5),0.0),1.0));
        }
        template<>
        uint8_t convert_c_from_double<false>(double value)
        {
            return static_cast<uint8_t>(224.0 * std::min(std::max((value+0.5),0.0),1.0))+16;
        }


        template<bool full_range>
        uint8_t convert_rgb_from_double(double value);

        template<>
        uint8_t convert_rgb_from_double<true>(double value)
        {
            return static_cast<uint8_t>(std::min(std::max(value,0.0),1.0)*255.0);
        }
        template<>
        uint8_t convert_rgb_from_double<false>(double value)
        {
            return static_cast<uint8_t>(std::min(std::max(value*255.0/235.0,0.0),1.0)*255.0);
        }

        template<class colorimetry, bool full_range>
        void set_yuv444_from_rgb(core::Plane::iterator& dest, const double r, const double g, const double b)
        {
            const double y = 	colorimetry::Wr() * r +
                                colorimetry::Wg() * g +
                                colorimetry::Wb() * b;
            *dest++ =  	convert_y_from_double<full_range>(y);
            *dest++ =   convert_c_from_double<full_range>(
                    (b - y) * colorimetry::Kb());
            *dest++ =   convert_c_from_double<full_range>(
                    (r - y) * colorimetry::Kr());
        }

        template<class colorimetry, bool full_range>
        void set_yuv422_from_rgb(core::Plane::iterator& dest, const double r, const double g, const double b,
                                 const double r2, const double g2, const double b2)
        {
            const double y = 	colorimetry::Wr() * r +
                                colorimetry::Wg() * g +
                                colorimetry::Wb() * b;
            const double u = (b - y) * colorimetry::Kb();
            const double v = (r - y) * colorimetry::Kr();
            const double y2 = 	colorimetry::Wr() * r2 +
                                 colorimetry::Wg() * g2 +
                                 colorimetry::Wb() * b2;
            const double u2 = (b2 - y2) * colorimetry::Kb();
            const double v2 = (r2 - y2) * colorimetry::Kr();
            *dest++ =  	convert_y_from_double<full_range>(y);
            *dest++ =   convert_c_from_double<full_range>((u+u2)/2);
            *dest++ =	convert_y_from_double<full_range>(y2);
            *dest++ =   convert_c_from_double<full_range>((v+v2)/2);
        }

        template<class colorimetry, bool full_range>
        void set_rgb_from_yuv(core::Plane::iterator& dest, const double y, const double u, const double v)
        {
            *dest++ =  	convert_rgb_from_double<full_range>(y + v / colorimetry::Kr());
            *dest++ =   convert_rgb_from_double<full_range>(y  - v*colorimetry::WrKrWg() - u*colorimetry::WbKbWg());
            *dest++ =   convert_rgb_from_double<full_range>(y + u / colorimetry::Kb());
        }

/* ***************************************************************************
 * 					Conversions
 *************************************************************************** */

// This has to be functor in order to use dispatch templates above.
        template<class colorimetry, bool full_range>
        struct convert_line_rgb_yuv444{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width; ++pixel) {
                    const double r = (*src++)/255.0;
                    const double g = (*src++)/255.0;
                    const double b = (*src++)/255.0;
                    set_yuv444_from_rgb<colorimetry, full_range>(dest, r, g, b);
                }
            }
        };


        template<>
        void convert_line<core::raw_format::rgb24, core::raw_format::yuv444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_rgb_yuv444>(src, dest, width, col, full_range);
        }

        template<class colorimetry, bool full_range>
        struct convert_line_rgba_yuv444{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width; ++pixel) {
                    const double r = (*src++)/255.0;
                    const double g = (*src++)/255.0;
                    const double b = (*src++)/255.0;
                    src++;
                    set_yuv444_from_rgb<colorimetry, full_range>(dest, r, g, b);
                }
            }
        };

        template<>
        void convert_line<core::raw_format::rgba32, core::raw_format::yuv444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_rgba_yuv444>(src, dest, width, col, full_range);
        }

        template<class colorimetry, bool full_range>
        struct convert_line_argb_yuv444{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width; ++pixel) {
                    src++;
                    const double r = (*src++)/255.0;
                    const double g = (*src++)/255.0;
                    const double b = (*src++)/255.0;
                    set_yuv444_from_rgb<colorimetry, full_range>(dest, r, g, b);
                }
            }
        };

        template<>
        void convert_line<core::raw_format::argb32, core::raw_format::yuv444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_argb_yuv444>(src, dest, width, col, full_range);
        }

        template<class colorimetry, bool full_range>
        struct convert_line_bgr_yuv444{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width; ++pixel) {
                    const double b = (*src++)/255.0;
                    const double g = (*src++)/255.0;
                    const double r = (*src++)/255.0;
                    set_yuv444_from_rgb<colorimetry, full_range>(dest, r, g, b);
                }
            }
        };

        template<>
        void convert_line<core::raw_format::bgr24, core::raw_format::yuv444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_bgr_yuv444>(src, dest, width, col, full_range);
        }

        template<class colorimetry, bool full_range>
        struct convert_line_bgra_yuv444{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width; ++pixel) {
                    const double b = (*src++)/255.0;
                    const double g = (*src++)/255.0;
                    const double r = (*src++)/255.0;
                    src++;
                    set_yuv444_from_rgb<colorimetry, full_range>(dest, r, g, b);
                }
            }
        };

        template<>
        void convert_line<core::raw_format::bgra32, core::raw_format::yuv444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_bgra_yuv444>(src, dest, width, col, full_range);
        }


        template<class colorimetry, bool full_range>
        struct convert_line_abgr_yuv444{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width; ++pixel) {
                    src++;
                    const double b = (*src++)/255.0;
                    const double g = (*src++)/255.0;
                    const double r = (*src++)/255.0;
                    set_yuv444_from_rgb<colorimetry, full_range>(dest, r, g, b);
                }
            }
        };

        template<>
        void convert_line<core::raw_format::abgr32, core::raw_format::yuv444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_abgr_yuv444>(src, dest, width, col, full_range);
        }

// YUV422
        template<class colorimetry, bool full_range>
        struct convert_line_rgb_yuv422{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width/2; ++pixel) {
                    const double r = (*src++)/255.0;
                    const double g = (*src++)/255.0;
                    const double b = (*src++)/255.0;
                    const double r2 = (*src++)/255.0;
                    const double g2 = (*src++)/255.0;
                    const double b2 = (*src++)/255.0;
                    set_yuv422_from_rgb<colorimetry, full_range>(dest, r, g, b, r2, g2, b2);
                }
            }
        };


        template<>
        void convert_line<core::raw_format::rgb24, core::raw_format::yuyv422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_rgb_yuv422>(src, dest, width, col, full_range);
        }

        template<class colorimetry, bool full_range>
        struct convert_line_rgba_yuv422{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width/2; ++pixel) {
                    const double r = (*src++)/255.0;
                    const double g = (*src++)/255.0;
                    const double b = (*src++)/255.0;
                    src++;
                    const double r2 = (*src++)/255.0;
                    const double g2 = (*src++)/255.0;
                    const double b2 = (*src++)/255.0;
                    src++;
                    set_yuv422_from_rgb<colorimetry, full_range>(dest, r, g, b, r2, g2, b2);
                }
            }
        };

        template<>
        void convert_line<core::raw_format::rgba32, core::raw_format::yuyv422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_rgba_yuv422>(src, dest, width, col, full_range);
        }

        template<class colorimetry, bool full_range>
        struct convert_line_argb_yuv422{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width/2; ++pixel) {
                    src++;
                    const double r = (*src++)/255.0;
                    const double g = (*src++)/255.0;
                    const double b = (*src++)/255.0;
                    src++;
                    const double r2 = (*src++)/255.0;
                    const double g2 = (*src++)/255.0;
                    const double b2 = (*src++)/255.0;
                    set_yuv422_from_rgb<colorimetry, full_range>(dest, r, g, b, r2, g2, b2);
                }
            }
        };

        template<>
        void convert_line<core::raw_format::argb32, core::raw_format::yuyv422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_argb_yuv422>(src, dest, width, col, full_range);
        }

        template<class colorimetry, bool full_range>
        struct convert_line_bgr_yuv422{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width/2; ++pixel) {
                    const double b = (*src++)/255.0;
                    const double g = (*src++)/255.0;
                    const double r = (*src++)/255.0;
                    const double b2 = (*src++)/255.0;
                    const double g2 = (*src++)/255.0;
                    const double r2 = (*src++)/255.0;
                    set_yuv422_from_rgb<colorimetry, full_range>(dest, r, g, b, r2, g2, b2);
                }
            }
        };

        template<>
        void convert_line<core::raw_format::bgr24, core::raw_format::yuyv422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_bgr_yuv422>(src, dest, width, col, full_range);
        }

        template<class colorimetry, bool full_range>
        struct convert_line_bgra_yuv422{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width/2; ++pixel) {
                    const double b = (*src++)/255.0;
                    const double g = (*src++)/255.0;
                    const double r = (*src++)/255.0;
                    src++;
                    const double b2 = (*src++)/255.0;
                    const double g2 = (*src++)/255.0;
                    const double r2 = (*src++)/255.0;
                    src++;
                    set_yuv422_from_rgb<colorimetry, full_range>(dest, r, g, b, r2, g2, b2);

                }
            }
        };

        template<>
        void convert_line<core::raw_format::bgra32, core::raw_format::yuyv422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_bgra_yuv422>(src, dest, width, col, full_range);
        }


        template<class colorimetry, bool full_range>
        struct convert_line_abgr_yuv422{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width/2; ++pixel) {
                    src++;
                    const double b = (*src++)/255.0;
                    const double g = (*src++)/255.0;
                    const double r = (*src++)/255.0;
                    src++;
                    const double b2 = (*src++)/255.0;
                    const double g2 = (*src++)/255.0;
                    const double r2 = (*src++)/255.0;
                    set_yuv422_from_rgb<colorimetry, full_range>(dest, r, g, b, r2, g2, b2);
                }
            }
        };

        template<>
        void convert_line<core::raw_format::abgr32, core::raw_format::yuyv422>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_abgr_yuv422>(src, dest, width, col, full_range);
        }


        template<class colorimetry, bool full_range>
        struct convert_line_yuv444_rgb{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width; ++pixel) {
                    const double y = (*src++)/255.0;
                    const double u = (*src++)/255.0 - 0.5;
                    const double v = (*src++)/255.0 - 0.5;
                    set_rgb_from_yuv<colorimetry, full_range>(dest, y, u, v);

                }
            }
        };

        template<>
        void convert_line<core::raw_format::yuv444, core::raw_format::rgb24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_yuv444_rgb>(src, dest, width, col, full_range);
        }

        template<class colorimetry, bool full_range>
        struct convert_line_yuv422_rgb{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width/2; ++pixel) {
                    const double y = (*src++)/255.0;
                    const double u = (*src++)/255.0 - 0.5;
                    const double y2 = (*src++)/255.0;
                    const double v = (*src++)/255.0 - 0.5;
                    set_rgb_from_yuv<colorimetry, full_range>(dest, y, u, v);
                    set_rgb_from_yuv<colorimetry, full_range>(dest, y2, u, v);

                }
            }
        };

        template<>
        void convert_line<core::raw_format::yuyv422, core::raw_format::rgb24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_yuv422_rgb>(src, dest, width, col, full_range);
        }
        template<class colorimetry, bool full_range>
        struct convert_line_uyvy422_rgb{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width/2; ++pixel) {
                    const double u = (*src++)/255.0 - 0.5;
                    const double y = (*src++)/255.0;
                    const double v = (*src++)/255.0 - 0.5;
                    const double y2 = (*src++)/255.0;
                    set_rgb_from_yuv<colorimetry, full_range>(dest, y, u, v);
                    set_rgb_from_yuv<colorimetry, full_range>(dest, y2, u, v);

                }
            }
        };

        template<>
        void convert_line<core::raw_format::uyvy422, core::raw_format::rgb24>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_uyvy422_rgb>(src, dest, width, col, full_range);
        }


        template<class colorimetry, bool full_range>
        struct convert_line_rgba_yuva4444{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width; ++pixel) {
                    const double r = (*src++)/255.0;
                    const double g = (*src++)/255.0;
                    const double b = (*src++)/255.0;
                    set_yuv444_from_rgb<colorimetry, full_range>(dest, r, g, b);
                    *dest++ = *src++;
                }
            }
        };

        template<>
        void convert_line<core::raw_format::rgba32, core::raw_format::yuva4444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_rgba_yuva4444>(src, dest, width, col, full_range);
        }

        template<class colorimetry, bool full_range>
        struct convert_line_bgra_yuva4444{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width; ++pixel) {
                    const double b = (*src++)/255.0;
                    const double g = (*src++)/255.0;
                    const double r = (*src++)/255.0;
                    set_yuv444_from_rgb<colorimetry, full_range>(dest, r, g, b);
                    *dest++ = *src++;
                }
            }
        };

        template<>
        void convert_line<core::raw_format::bgra32, core::raw_format::yuva4444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_bgra_yuva4444>(src, dest, width, col, full_range);
        }

        template<class colorimetry, bool full_range>
        struct convert_line_argb_yuva4444{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width; ++pixel) {
                    *dest++ = *src++;
                    const double r = (*src++)/255.0;
                    const double g = (*src++)/255.0;
                    const double b = (*src++)/255.0;
                    set_yuv444_from_rgb<colorimetry, full_range>(dest, r, g, b);
                }
            }
        };

        template<>
        void convert_line<core::raw_format::argb32, core::raw_format::yuva4444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_argb_yuva4444>(src, dest, width, col, full_range);
        }
        template<class colorimetry, bool full_range>
        struct convert_line_abgr_yuva4444{
            static void eval
                    (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width)
            {
                for (size_t pixel = 0; pixel < width; ++pixel) {
                    *dest++ = *src++;
                    const double b = (*src++)/255.0;
                    const double g = (*src++)/255.0;
                    const double r = (*src++)/255.0;
                    set_yuv444_from_rgb<colorimetry, full_range>(dest, r, g, b);
                }
            }
        };

        template<>
        void convert_line<core::raw_format::abgr32, core::raw_format::yuva4444>
                (core::Plane::const_iterator src, core::Plane::iterator dest, size_t width, const YuriConvertor& conv)
        {
            colorimetry_t col = conv.get_colorimetry();
            bool full_range = conv.get_full_range();
            convert_rgb_yuv_dispatch<convert_line_abgr_yuva4444>(src, dest, width, col, full_range);
        }

        converter_map get_converters_yuv_rgb() {
            static std::map<format_pair_t, std::pair<converter_t, size_t>> converters_yuv_rgb = {
                    ADD_CONVERSION(core::raw_format::rgb24, core::raw_format::yuv444, 20)
                    ADD_CONVERSION(core::raw_format::rgba32, core::raw_format::yuv444, 20)
                    ADD_CONVERSION(core::raw_format::argb32, core::raw_format::yuv444, 20)
                    ADD_CONVERSION(core::raw_format::bgr24, core::raw_format::yuv444, 20)
                    ADD_CONVERSION(core::raw_format::bgra32, core::raw_format::yuv444, 20)
                    ADD_CONVERSION(core::raw_format::abgr32, core::raw_format::yuv444, 20)
                    ADD_CONVERSION(core::raw_format::rgb24, core::raw_format::yuyv422, 25)
                    ADD_CONVERSION(core::raw_format::rgba32, core::raw_format::yuyv422, 25)
                    ADD_CONVERSION(core::raw_format::argb32, core::raw_format::yuyv422, 25)
                    ADD_CONVERSION(core::raw_format::bgr24, core::raw_format::yuyv422, 25)
                    ADD_CONVERSION(core::raw_format::bgra32, core::raw_format::yuyv422, 25)
                    ADD_CONVERSION(core::raw_format::abgr32, core::raw_format::yuyv422, 25)
                    ADD_CONVERSION(core::raw_format::rgba32, core::raw_format::yuva4444, 25)
                    ADD_CONVERSION(core::raw_format::bgra32, core::raw_format::yuva4444, 25)
                    ADD_CONVERSION(core::raw_format::argb32, core::raw_format::yuva4444, 25)
                    ADD_CONVERSION(core::raw_format::abgr32, core::raw_format::yuva4444, 25)
                    ADD_CONVERSION(core::raw_format::yuv444, core::raw_format::rgb24, 20)
                    ADD_CONVERSION(core::raw_format::yuyv422, core::raw_format::rgb24, 25)
                    ADD_CONVERSION(core::raw_format::uyvy422, core::raw_format::rgb24, 25)
            };
            return converters_yuv_rgb;
        }
    }
}