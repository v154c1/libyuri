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
    p.set_description("LinkyOutput");
    p["url"]["API url base path"]                 = "https://service.iim.cz/linkyapi";
    p["key"]["API key"]                           = "";
    p["resolution"]["Display resolution"]         = "5x204";
    p["use_rgbw"]["Use RGBW values"]              = false;
    p["w_value"]["Fixed W value for rgbw format"] = 0;
    return p;
}

namespace {
}

LinkyOutput::LinkyOutput(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters)
    : base_type(log_, parent, std::string("linky_output")), event::BasicEventConsumer(log)
{
    IOTHREAD_INIT(parameters)
    set_supported_formats({ core::raw_format::rgb24 });
}

LinkyOutput::~LinkyOutput() noexcept
{
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

std::string fill_rgb(const core::pRawVideoFrame& frame, const resolution_t& resolution)
{
    const auto res  = frame->get_resolution();
    const auto dres = resolution_t{ std::min(res.width, resolution.width), std::min(res.height, resolution.height) };

    auto data     = std::string(resolution.width * resolution.height * 6, '0');
    auto pdata    = &data[0];
    auto raw_data = PLANE_RAW_DATA(frame, 0);
    for (auto y : irange(dres.height)) {
        auto dstart = raw_data + y * PLANE_DATA(frame, 0).get_line_size();
        for (auto x : irange(dres.width)) {
            auto p = pdata + 6 * (x * resolution.height + y);
            write_8bit(p + 0, *dstart++);
            write_8bit(p + 2, *dstart++);
            write_8bit(p + 4, *dstart++);
        }
    }
    return data;
}

std::string fill_rgbw(const core::pRawVideoFrame& frame, const resolution_t& resolution, const uint8_t& w_value)
{
    const auto res  = frame->get_resolution();
    const auto dres = resolution_t{ std::min(res.width, resolution.width), std::min(res.height, resolution.height) };

    auto data     = std::string(resolution.width * resolution.height * 8, '0');
    auto pdata    = &data[0];
    auto raw_data = PLANE_RAW_DATA(frame, 0);
    for (auto y : irange(dres.height)) {
        auto dstart = raw_data + y * PLANE_DATA(frame, 0).get_line_size();
        for (auto x : irange(dres.width)) {
            auto p = pdata + 8 * (x * resolution.height + y);
            write_8bit(p + 0, *dstart++);
            write_8bit(p + 2, *dstart++);
            write_8bit(p + 4, *dstart++);
            write_8bit(p + 6, w_value);
        }
    }
    return data;
}
}
core::pFrame LinkyOutput::do_special_single_step(core::pRawVideoFrame frame)
{
	process_events();
    auto data = use_rgbw_ ? fill_rgbw(frame, resolution_, w_value_) : fill_rgb(frame, resolution_);

    //	log[log::info] << data;
    Json::Value root;
    root["colourScheme"]   = use_rgbw_ ? "rgbw" : "rgb";
    root["colourDataType"] = "many";
    root["colourData"]     = data;
    std::stringstream ss;
    ss << root;
    upload_json(api_path_ + "/lights/all", ss.str(), key_);
    return {};
}

bool LinkyOutput::set_param(const core::Parameter& param)
{
    if (assign_parameters(param)    //
        (api_path_, "url")          //
        (key_, "key")               //
        (resolution_, "resolution") //
        (use_rgbw_, "use_rgbw")     //
        (w_value_, "w_value"))
        return true;

    return base_type::set_param(param);
}

bool LinkyOutput::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
    if (assign_events(event_name, event) //
        (api_path_, "url")               //
        (key_, "key")                    //
        (resolution_, "resolution")      //
        (use_rgbw_, "use_rgbw")          //
        (w_value_, "w_value"))
        return true;
    return false;
}

} /* namespace linky_output */
} /* namespace yuri */
