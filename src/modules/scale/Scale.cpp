/*!
 * @file 		Scale.cpp
 * @author 		<Your name>
 * @date		09.04.2014
 * @copyright	Institute of Intermedia, 2013
 * 				Distributed BSD License
 *
 */

#include "Scale.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/utils/assign_events.h"

namespace yuri {
namespace scale {

IOTHREAD_GENERATOR(Scale)

MODULE_REGISTRATION_BEGIN("scale")
REGISTER_IOTHREAD("scale", Scale)
MODULE_REGISTRATION_END()

core::Parameters Scale::configure()
{
    core::Parameters p = base_type::configure();
    p.set_description("Scale");
    p["resolution"]["Resolution to scale to"] = resolution_t{ 800, 600 };
    p["fast"]["Enable fast scaling"]          = true;
    return p;
}

Scale::Scale(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters)
    : base_type(log_, parent, std::string("scale")), event::BasicEventConsumer(log), resolution_(resolution_t{ 800, 600 }), fast_(true)
{
    IOTHREAD_INIT(parameters)
    using namespace core::raw_format;
    set_supported_formats({ rgb24, bgr24, rgba32, argb32, bgra32, abgr32, yuv444, yuyv422, yvyu422, uyvy422, vyuy422, yuva4444 });
    //	set_latency(1_ms);
}

Scale::~Scale() noexcept
{
}

namespace {

template <size_t pixel_size> struct scale_line_bilinear {
    inline static void eval(uint8_t* it, const uint8_t* top, const uint8_t* bottom, const dimension_t new_width, const dimension_t old_width,
                            const double unscale_x, const double y_ratio)
    {
        const double y_ratio2 = 1.0 - y_ratio;
        for (dimension_t pixel = 0; pixel < new_width - 1; ++pixel) {
            const dimension_t left     = static_cast<dimension_t>(pixel * unscale_x);
            const dimension_t right    = left + 1;
            const double      x_ratio  = pixel * unscale_x - left;
            const double      x_ratio2 = 1.0 - x_ratio;
            for (size_t i = 0; i < pixel_size; ++i) {
                *it++ = static_cast<uint8_t>(top[left * pixel_size + i] * x_ratio2 * y_ratio2 + top[right * pixel_size + i] * x_ratio * y_ratio2
                                             + bottom[left * pixel_size + i] * x_ratio2 * y_ratio + bottom[right * pixel_size + i] * x_ratio * y_ratio);
            }
        }
        for (size_t i = 0; i < pixel_size; ++i) {
            *it++ = static_cast<uint8_t>(top[(old_width - 1) * pixel_size + i] * y_ratio2 + bottom[(old_width - 1) * pixel_size + i] * y_ratio);
        }
    }
};

template <size_t pixel_size> struct scale_line_bilinear_fast {
    inline static void eval(uint8_t* it, const uint8_t* top, const uint8_t* bottom, const dimension_t new_width, const dimension_t old_width,
                            const uint64_t unscale_x, const uint64_t y_ratio)
    {
        const uint64_t y_ratio2 = 256 - y_ratio;
        for (dimension_t pixel = 0; pixel < new_width - 1; ++pixel) {
            const dimension_t left     = pixel * unscale_x;
            const dimension_t right    = left + 1;
            const uint64_t    x_ratio  = pixel * unscale_x - left;
            const uint64_t    x_ratio2 = 256 - x_ratio;
            for (size_t i = 0; i < pixel_size; ++i) {
                *it++ = static_cast<uint8_t>((top[left / 256 * pixel_size + i] * x_ratio2 * y_ratio2 + top[right / 256 * pixel_size + i] * x_ratio * y_ratio2
                                              + bottom[left / 256 * pixel_size + i] * x_ratio2 * y_ratio
                                              + bottom[right / 256 * pixel_size + i] * x_ratio * y_ratio)
                                             / 65536);
            }
        }
        for (size_t i = 0; i < pixel_size; ++i) {
            *it++ = static_cast<uint8_t>((top[(old_width - 1) * pixel_size + i] * y_ratio2 + bottom[(old_width - 1) * pixel_size + i] * y_ratio) / 256);
        }
    }
};
inline uint8_t get_y(const dimension_t pixel, const double unscale_x, const uint8_t* top, const uint8_t* bottom, const double y_ratio, const double y_ratio2)
{
    const dimension_t left  = static_cast<dimension_t>(pixel * unscale_x);
    const dimension_t right = left + 1;

    const double x_ratio  = pixel * unscale_x - left;
    const double x_ratio2 = 1.0 - x_ratio;
    return static_cast<uint8_t>(top[left * 2 + 0] * x_ratio2 * y_ratio2 + top[right * 2 + 0] * x_ratio * y_ratio2 + bottom[left * 2 + 0] * x_ratio2 * y_ratio
                                + bottom[right * 2 + 0] * x_ratio * y_ratio);
}
template <size_t adjust>
inline uint8_t get_uv(const dimension_t pixel, const double unscale_x, const uint8_t* top, const uint8_t* bottom, const double y_ratio, const double y_ratio2)
{
    const dimension_t left0 = static_cast<dimension_t>(pixel * unscale_x);
    const dimension_t left  = (left0 & ~1) + adjust;
    const dimension_t right = left + 2;

    const double x_ratio  = (pixel * unscale_x - left0 + (left0 & 1)) / 2.0;
    const double x_ratio2 = 1.0 - x_ratio;
    return static_cast<uint8_t>(top[left * 2 + 1] * x_ratio2 * y_ratio2 + top[right * 2 + 1] * x_ratio * y_ratio2 + bottom[left * 2 + 1] * x_ratio2 * y_ratio
                                + bottom[right * 2 + 1] * x_ratio * y_ratio);
}

struct scale_line_bilinear_yuyv {
    inline static void eval(uint8_t* it, const uint8_t* top, const uint8_t* bottom, const dimension_t new_width, const dimension_t /*old_width*/,
                            const double unscale_x, const double y_ratio)
    {
        const double y_ratio2 = 1.0 - y_ratio;
        for (dimension_t pixel = 0; pixel < new_width - 2; pixel += 2) {
            *it++ = get_y(pixel, unscale_x, top, bottom, y_ratio, y_ratio2);
            *it++ = get_uv<0>(pixel, unscale_x, top, bottom, y_ratio, y_ratio2);
            *it++ = get_y(pixel + 1, unscale_x, top, bottom, y_ratio, y_ratio2);
            *it++ = get_uv<1>(pixel + 1, unscale_x, top, bottom, y_ratio, y_ratio2);
        }
        *it++ = get_y((new_width - 2), unscale_x, top, bottom, y_ratio, y_ratio2);
        *it++ = get_uv<0>((new_width - 2), unscale_x, top, bottom, y_ratio, y_ratio2);
        *it++ = get_y((new_width - 1), unscale_x, top, bottom, y_ratio, y_ratio2);
        *it++ = get_uv<1>((new_width - 1), unscale_x, top, bottom, y_ratio, y_ratio2);
    }
};
struct scale_line_bilinear_uyvy {
    inline static void eval(uint8_t* it, const uint8_t* top, const uint8_t* bottom, const dimension_t new_width, const dimension_t /*old_width*/,
                            const double unscale_x, const double y_ratio)
    {
        const double y_ratio2 = 1.0 - y_ratio;
        for (dimension_t pixel = 0; pixel < new_width - 2; pixel += 2) {
            *it++ = get_uv<0>(pixel, unscale_x, top - 1, bottom - 1, y_ratio, y_ratio2);
            *it++ = get_y(pixel, unscale_x, top + 1, bottom + 1, y_ratio, y_ratio2);
            *it++ = get_uv<1>(pixel + 1, unscale_x, top - 1, bottom - 1, y_ratio, y_ratio2);
            *it++ = get_y(pixel + 1, unscale_x, top + 1, bottom + 1, y_ratio, y_ratio2);
        }
        *it++ = get_uv<0>((new_width - 2), unscale_x, top - 1, bottom - 1, y_ratio, y_ratio2);
        *it++ = get_y((new_width - 2), unscale_x, top + 1, bottom + 1, y_ratio, y_ratio2);
        *it++ = get_uv<1>((new_width - 1), unscale_x, top - 1, bottom - 1, y_ratio, y_ratio2);
        *it++ = get_y((new_width - 1), unscale_x, top + 1, bottom + 1, y_ratio, y_ratio2);
    }
};

inline uint8_t get_y_fast(const dimension_t pixel, const uint64_t unscale_x, const uint8_t* top, const uint8_t* bottom, const uint64_t y_ratio,
                          const uint64_t y_ratio2)
{
    const dimension_t left  = pixel * unscale_x;
    const dimension_t right = left + 256;

    const uint64_t x_ratio  = pixel * unscale_x - left;
    const uint64_t x_ratio2 = 256 - x_ratio;
    //	std::cout << "pixel: " << pixel << ", left:"<<left << ", leftx: " << left/256 * 2 <<std::endl;
    return static_cast<uint8_t>((top[left / 256 * 2 + 0] * x_ratio2 * y_ratio2 + top[right / 256 * 2 + 0] * x_ratio * y_ratio2
                                 + bottom[left / 256 * 2 + 0] * x_ratio2 * y_ratio + bottom[right / 256 * 2 + 0] * x_ratio * y_ratio)
                                / 65536);
}
template <size_t adjust>
inline uint8_t get_uv_fast(const dimension_t pixel, const uint64_t unscale_x, const uint8_t* top, const uint8_t* bottom, const uint64_t y_ratio,
                           const uint64_t y_ratio2)
{
    const dimension_t left0 = pixel * unscale_x;
    const dimension_t left  = (left0 & ~0x1FF) + 256 * adjust;
    const dimension_t right = left + 2 * 256;

    const uint64_t x_ratio  = (pixel * unscale_x - left0 + (left0 & 0x1FF)) / 2;
    const uint64_t x_ratio2 = 256 - x_ratio;
    return static_cast<uint8_t>((top[left / 256 * 2 + 1] * x_ratio2 * y_ratio2 + top[right / 256 * 2 + 1] * x_ratio * y_ratio2
                                 + bottom[left / 256 * 2 + 1] * x_ratio2 * y_ratio + bottom[right / 256 * 2 + 1] * x_ratio * y_ratio)
                                / 65536);
}

struct scale_line_bilinear_yuyv_fast {
    inline static void eval(uint8_t* it, const uint8_t* top, const uint8_t* bottom, const dimension_t new_width, const dimension_t /*old_width*/,
                            const uint64_t unscale_x, const uint64_t y_ratio)
    {
        const uint64_t y_ratio2 = 256 - y_ratio;
        for (dimension_t pixel = 0; pixel < new_width - 2; pixel += 2) {
            *it++ = get_y_fast(pixel, unscale_x, top, bottom, y_ratio, y_ratio2);
            *it++ = get_uv_fast<0>(pixel, unscale_x, top, bottom, y_ratio, y_ratio2);
            *it++ = get_y_fast(pixel + 1, unscale_x, top, bottom, y_ratio, y_ratio2);
            *it++ = get_uv_fast<1>(pixel + 1, unscale_x, top, bottom, y_ratio, y_ratio2);
        }
        *it++ = get_y_fast((new_width - 2), unscale_x, top, bottom, y_ratio, y_ratio2);
        *it++ = get_uv_fast<0>((new_width - 2), unscale_x, top, bottom, y_ratio, y_ratio2);
        *it++ = get_y_fast((new_width - 1), unscale_x, top, bottom, y_ratio, y_ratio2);
        *it++ = get_uv_fast<1>((new_width - 1), unscale_x, top, bottom, y_ratio, y_ratio2);
    }
};

struct scale_line_bilinear_uyvy_fast {
    inline static void eval(uint8_t* it, const uint8_t* top, const uint8_t* bottom, const dimension_t new_width, const dimension_t /*old_width*/,
                            const uint64_t unscale_x, const uint64_t y_ratio)
    {
        const uint64_t y_ratio2 = 256 - y_ratio;
        for (dimension_t pixel = 0; pixel < new_width - 2; pixel += 2) {
            // Using top - 1 and bottom - 1 to reuse methods for yuv
            *it++ = get_uv_fast<0>(pixel, unscale_x, top - 1, bottom - 1, y_ratio, y_ratio2);
            // Using top + 1 and bottom + 1 to reuse methods for yuv
            *it++ = get_y_fast(pixel, unscale_x, top + 1, bottom + 1, y_ratio, y_ratio2);
            *it++ = get_uv_fast<1>(pixel + 1, unscale_x, top - 1, bottom - 1, y_ratio, y_ratio2);
            *it++ = get_y_fast(pixel + 1, unscale_x, top + 1, bottom + 1, y_ratio, y_ratio2);
        }
        *it++ = get_uv_fast<0>((new_width - 2), unscale_x, top - 1, bottom - 1, y_ratio, y_ratio2);
        *it++ = get_y_fast((new_width - 2), unscale_x, top + 1, bottom + 1, y_ratio, y_ratio2);
        *it++ = get_uv_fast<1>((new_width - 1), unscale_x, top - 1, bottom - 1, y_ratio, y_ratio2);
        *it++ = get_y_fast((new_width - 1), unscale_x, top + 1, bottom + 1, y_ratio, y_ratio2);
    }
};

template <class kernel> core::pRawVideoFrame scale_image(const core::pRawVideoFrame& frame, const resolution_t new_resolution)
{
    auto           outframe     = core::RawVideoFrame::create_empty(frame->get_format(), new_resolution);
    const auto     res          = frame->get_resolution();
    const double   unscale_x    = static_cast<double>(res.width - 1) / (new_resolution.width - 1);
    const double   unscale_y    = static_cast<double>(res.height - 1) / (new_resolution.height - 1);
    const auto     linesize_in  = PLANE_DATA(frame, 0).get_line_size();
    const auto     linesize_out = PLANE_DATA(outframe, 0).get_line_size();
    const uint8_t* it_in        = PLANE_RAW_DATA(frame, 0);
    uint8_t*       it           = PLANE_RAW_DATA(outframe, 0);

    for (dimension_t line = 0; line < new_resolution.height - 1; ++line) {
        const dimension_t top     = static_cast<dimension_t>(line * unscale_y);
        const dimension_t bottom  = top + 1;
        const double      y_ratio = line * unscale_y - top;
        kernel::eval(it, it_in + top * linesize_in, it_in + bottom * linesize_in, new_resolution.width, res.width, unscale_x, y_ratio);

        it += linesize_out;
    }
    kernel::eval(PLANE_RAW_DATA(outframe, 0) + (new_resolution.height - 1) * linesize_out, PLANE_RAW_DATA(frame, 0) + (res.height - 1) * linesize_in,
                 PLANE_RAW_DATA(frame, 0) + (res.height - 1) * linesize_in, new_resolution.width, res.width, unscale_x, 0.0);
    outframe->copy_video_params(*frame);
    return outframe;
}

template <class kernel> core::pRawVideoFrame scale_image_fast(const core::pRawVideoFrame& frame, const resolution_t new_resolution)
{
    auto           outframe     = core::RawVideoFrame::create_empty(frame->get_format(), new_resolution);
    const auto     res          = frame->get_resolution();
    const uint64_t unscale_x    = 256 * (res.width - 1) / (new_resolution.width - 1);
    const uint64_t unscale_y    = 256 * (res.height - 1) / (new_resolution.height - 1);
    const auto     linesize_in  = PLANE_DATA(frame, 0).get_line_size();
    const auto     linesize_out = PLANE_DATA(outframe, 0).get_line_size();
    const uint8_t* it_in        = PLANE_RAW_DATA(frame, 0);
    uint8_t*       it           = PLANE_RAW_DATA(outframe, 0);

    for (dimension_t line = 0; line < new_resolution.height - 1; ++line) {
        const dimension_t top     = line * unscale_y;
        const dimension_t bottom  = top + 256;
        const uint64_t    y_ratio = line * unscale_y - top;
        kernel::eval(it, it_in + top / 256 * linesize_in, it_in + bottom / 256 * linesize_in, new_resolution.width, res.width, unscale_x, y_ratio);

        it += linesize_out;
    }
    kernel::eval(PLANE_RAW_DATA(outframe, 0) + (new_resolution.height - 1) * linesize_out, PLANE_RAW_DATA(frame, 0) + (res.height - 1) * linesize_in,
                 PLANE_RAW_DATA(frame, 0) + (res.height - 1) * linesize_in, new_resolution.width, res.width, unscale_x, 0.0);
    outframe->copy_video_params(*frame);
    return outframe;
}
}

core::pFrame Scale::do_special_single_step(core::pRawVideoFrame frame)
{
    process_events();
    if (!resolution_)
    	return {};
    if (frame->get_resolution() == resolution_)
        return frame;
    // Simple sanity check
    if (resolution_.width > 1e5 || resolution_.height > 1e5)
    	return {};
    using namespace core::raw_format;
    if (fast_) {
        switch (frame->get_format()) {
        case rgb24:
        case bgr24:
        case yuv444:
            return scale_image_fast<scale_line_bilinear_fast<3>>(frame, resolution_);

        case rgba32:
        case argb32:
        case bgra32:
        case abgr32:
        case yuva4444:
            return scale_image_fast<scale_line_bilinear_fast<4>>(frame, resolution_);
        case yuyv422:
        case yvyu422:
            return scale_image_fast<scale_line_bilinear_yuyv_fast>(frame, resolution_);
        case uyvy422:
        case vyuy422:
            return scale_image_fast<scale_line_bilinear_uyvy_fast>(frame, resolution_);
        }
    } else {
        switch (frame->get_format()) {
        case rgb24:
        case bgr24:
        case yuv444:
            return scale_image<scale_line_bilinear<3>>(frame, resolution_);

        case rgba32:
        case argb32:
        case bgra32:
        case abgr32:
        case yuva4444:
            return scale_image<scale_line_bilinear<4>>(frame, resolution_);
        case yuyv422:
        case yvyu422:
            return scale_image<scale_line_bilinear_yuyv>(frame, resolution_);
        case uyvy422:
        case vyuy422:
            return scale_image<scale_line_bilinear_uyvy>(frame, resolution_);
        }
    }
    return {};
}
bool Scale::set_param(const core::Parameter& param)
{
    if (assign_parameters(param)(resolution_, "resolution")(fast_, "fast"))
        return true;
    return base_type::set_param(param);
}

bool Scale::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
    if (assign_events(event_name, event)(resolution_, "resolution")(fast_, "fast"))
        return true;
    return false;
}
} /* namespace scale */
} /* namespace yuri */
