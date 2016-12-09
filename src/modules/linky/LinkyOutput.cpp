/*!
 * @file 		LinkyOutput.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		26.09.2016
 * @copyright	Institute of Intermedia, CTU in Prague, 2016
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "../linky/LinkyOutput.h"

#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/utils/irange.h"
#include "yuri/core/utils/assign_events.h"
#include "linky_common.h"
#include "json_helpers.h"
#include <sstream>

namespace yuri {
namespace linky {

IOTHREAD_GENERATOR(LinkyOutput)

core::Parameters LinkyOutput::configure()
{
    core::Parameters p = base_type::configure();
    p.set_description("Uploads images to Linky (Facade at FEE, CTU in Prague)");
    p["url"]["API url base path"]                                                                                = "https://service.iim.cz/linkyapi";
    p["key"]["API key"]                                                                                          = "";
    p["resolution"]["Display resolution"]                                                                        = "5x204";
    p["use_rgbw"]["Use RGBW values"]                                                                             = false;
    p["w_value"]["Fixed W value for rgbw format"]                                                                = 0;
    p["alpha_as_white"]["Use RGBA format as RGBW"]                                                               = false;
    p["sample"]["Use sampling (true) or take first pixels (false)"]                                              = false;
    p["sample_border"]["Width of border at each side relative to space between columns (only when sample=true)"] = 0.5;
    p["async"]["Upload data asynchronously. Set to 0 for synchronous upload, 1 for asynchronous and 2 for aggressive asynchronous upload"] = 0;
    p["enable_output"]["Enable output ot linky API"]                                                                                       = true;
    return p;
}

namespace {
}

LinkyOutput::LinkyOutput(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters)
    : base_type(log_, parent, std::string("linky_output")), event::BasicEventConsumer(log)
{
    IOTHREAD_INIT(parameters)
    if (alpha_as_white_) {
        set_supported_formats({ core::raw_format::rgb24, core::raw_format::rgba32 });
    } else {
        set_supported_formats({ core::raw_format::rgb24 });
    }
}

LinkyOutput::~LinkyOutput() noexcept
{
    if (async_result_.valid()) {
        async_result_.wait_for(std::chrono::milliseconds(500));
    }
}

namespace {
void write_4bit(char* p, int8_t value)
{
    value = value & 0xf;
    *p    = value < 10 ? value + '0' : value + 'a' - 10;
}
void write_8bit(char* p, int8_t value)
{
    write_4bit(p, value >> 4);
    write_4bit(p + 1, value);
}

struct RGBKernel {
    constexpr static size_t width     = 6;
    constexpr static size_t src_width = 3;
    template <class Iter>
    static void write(char* p, Iter& dstart, uint8_t)
    {
        write_8bit(p + 0, *dstart++);
        write_8bit(p + 2, *dstart++);
        write_8bit(p + 4, *dstart++);
    }
};

struct RGBWKernel {
    constexpr static size_t width     = 8;
    constexpr static size_t src_width = 3;
    template <class Iter>
    static void write(char* p, Iter& dstart, uint8_t w_value)
    {
        write_8bit(p + 0, *dstart++);
        write_8bit(p + 2, *dstart++);
        write_8bit(p + 4, *dstart++);
        write_8bit(p + 6, w_value);
    }
};

struct RGBAKernel {
    constexpr static size_t width     = 8;
    constexpr static size_t src_width = 4;
    template <class Iter>
    static void write(char* p, Iter& dstart, uint8_t)
    {
        write_8bit(p + 0, *dstart++);
        write_8bit(p + 2, *dstart++);
        write_8bit(p + 4, *dstart++);
        write_8bit(p + 6, *dstart++);
    }
};

template <class Kernel>
struct SimpleSampler {
    SimpleSampler(const resolution_t& resolution, uint8_t w_value) : resolution(resolution), w_value(w_value) {}

    resolution_t resolution;
    uint8_t      w_value;

    std::string fill(const core::pRawVideoFrame& frame)
    {
        const auto res  = frame->get_resolution();
        const auto dres = resolution_t{ std::min(res.width, resolution.width), std::min(res.height, resolution.height) };

        auto data     = std::string(resolution.width * resolution.height * Kernel::width, '0');
        auto pdata    = &data[0];
        auto raw_data = PLANE_RAW_DATA(frame, 0);
        for (auto y : irange(dres.height)) {
            auto dstart = raw_data + y * PLANE_DATA(frame, 0).get_line_size();
            for (auto x : irange(dres.width)) {
                auto p = pdata + 6 * (x * resolution.height + y);
                Kernel::write(p, dstart, w_value);
            }
        }
        return data;
    }
};

template <class Kernel>
struct IntervalSampler {
    IntervalSampler(const resolution_t& resolution, uint8_t w_value, float sample_border)
        : resolution(resolution), w_value(w_value), sample_border(sample_border)
    {
    }

    resolution_t resolution;
    uint8_t      w_value;
    float        sample_border;

    std::string fill(const core::pRawVideoFrame& frame)
    {
        const auto res  = frame->get_resolution();
        const auto dres = resolution_t{ resolution.width, std::min(res.height, resolution.height) };

        auto       data         = std::string(resolution.width * resolution.height * Kernel::width, '0');
        auto       pdata        = &data[0];
        auto       raw_data     = PLANE_RAW_DATA(frame, 0);
        const auto column_width = (res.width - 1) / (2.0f * sample_border + resolution.width - 1);
        for (auto y : irange(dres.height)) {
            auto dstart = raw_data + y * PLANE_DATA(frame, 0).get_line_size();
            for (auto x : irange(dres.width)) {
                auto xstart = dstart + static_cast<size_t>((x + sample_border) * column_width) * Kernel::src_width;
                auto p      = pdata + 6 * (x * resolution.height + y);
                Kernel::write(p, xstart, w_value);
            }
        }
        return data;
    }
};

template <class Kernel>
std::string dispatch_sampler(const resolution_t& resolution, const uint8_t w_value, bool sample, float sample_border, const core::pRawVideoFrame& frame)
{
    if (!sample) {
        return SimpleSampler<Kernel>(resolution, w_value).fill(frame);
    } else {
        return IntervalSampler<Kernel>(resolution, w_value, sample_border).fill(frame);
    }
}
}

core::pFrame LinkyOutput::do_special_single_step(core::pRawVideoFrame frame)
{
    process_events();
    const bool rgba_input = frame->get_format() == core::raw_format::rgba32;
    const bool rgbw       = use_rgbw_ || rgba_input;
    auto       data       = rgba_input ? dispatch_sampler<RGBAKernel>(resolution_, w_value_, sample_, sample_border_, frame)
                           : use_rgbw_ ? dispatch_sampler<RGBWKernel>(resolution_, w_value_, sample_, sample_border_, frame)
                                       : dispatch_sampler<RGBKernel>(resolution_, w_value_, sample_, sample_border_, frame);

    //	log[log::info] << data;
    Json::Value root;
    root["colourScheme"]   = rgbw ? "rgbw" : "rgb";
    root["colourDataType"] = "many";
    root["colourData"]     = data;
    std::stringstream ss;
    ss << root;
    if (async_ < 2 && async_result_.valid()) {
        async_result_.get();
    }
    if (enable_output_) {
        if (async_ < 1) {
            upload_json(api_path_ + "/lights/all", ss.str(), key_);
        } else {
            auto payload  = ss.str();
            async_result_ = std::async(std::launch::async, [this, payload]() { upload_json(api_path_ + "/lights/all", payload, key_); });
        }
    }
    return {};
}

bool LinkyOutput::set_param(const core::Parameter& param)
{
    if (assign_parameters(param)            //
        (api_path_, "url")                  //
        (key_, "key")                       //
        (resolution_, "resolution")         //
        (use_rgbw_, "use_rgbw")             //
        (w_value_, "w_value")               //
        (alpha_as_white_, "alpha_as_white") //
        (sample_, "sample")                 //
        (sample_border_, "sample_border")   //
        (async_, "async")                   //
        (enable_output_, "enable_output"))
        return true;

    return base_type::set_param(param);
}

bool LinkyOutput::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
    if (assign_events(event_name, event)    //
        (api_path_, "url")                  //
        (key_, "key")                       //
        (resolution_, "resolution")         //
        (use_rgbw_, "use_rgbw")             //
        (w_value_, "w_value")               //
        (alpha_as_white_, "alpha_as_white") //
        (sample_, "sample")                 //
        (sample_border_, "sample_border")   //
        (async_, "async")                   //
        (enable_output_, "enable_output")) {

        return true;
    }
    return false;
}

} /* namespace linky_output */
} /* namespace yuri */
