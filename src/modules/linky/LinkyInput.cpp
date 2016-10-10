/*!
 * @file 		LinkyInput.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		26.09.2016
 * @copyright	Institute of Intermedia, CTU in Prague, 2016
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "../linky/LinkyInput.h"

#include "yuri/core/Module.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/utils/irange.h"
#include "linky_common.h"
#include "json_helpers.h"
#include <sstream>
namespace yuri {
namespace linky {

IOTHREAD_GENERATOR(LinkyInput)

core::Parameters LinkyInput::configure()
{
    core::Parameters p = core::IOThread::configure();
    p.set_description("LinkyInput");
    p["url"]["API url base path"]         = "https://service.iim.cz/linkyapi";
    p["key"]["API key"]                   = "";
    p["resolution"]["Display resolution"] = "5x204";
    return p;
}

LinkyInput::LinkyInput(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters)
    : core::IOThread(log_, parent, 0, 1, std::string("linky_input"))
{
    IOTHREAD_INIT(parameters)
}

LinkyInput::~LinkyInput() noexcept
{
}

namespace {
int8_t get_4bit_value(const char* p)
{
	const auto& c = *p;
	if (c >= '0' && c <= '9') {
		return c-'0';
	}
	if (c >= 'a' && c <= 'f') {
		return 10 + c-'a';
	}
	if (c >= 'A' && c <= 'F') {
		return 10 + c-'A';
	}
	return 0;
}
int8_t get_8bit_value(const char* p)
{
	return (get_4bit_value(p) << 4) | get_4bit_value(p+1);
}
}
void LinkyInput::run()
{
    while (still_running()) {
    	sleep(10_ms);
        auto data = download_url(api_path_ + "/lights", key_);
//        log[log::info] << "Received " << data.size() << " bytes";
        Json::Value root;
        std::stringstream ss(data);
        ss >> root;
        const auto values = get_nested_value_or_default<std::string>(root, "nic", "data");
        log[log::verbose_debug] << "Values (" << values.size() << "): " << values;
        if (values.size() != resolution_.width * resolution_.height * 8) {
        	log[log::error] << "Wrong dimensions from server, ignoring";
        	continue;
        }
        auto frame = yuri::core::RawVideoFrame::create_empty(yuri::core::raw_format::rgb24, resolution_);
        auto fdata = PLANE_RAW_DATA(frame, 0);

        for (auto y: irange(resolution_.height)) {
        	for (auto x: irange(resolution_.width)) {
        		auto p = values.data()+8*(resolution_.height * x + y);
        		*fdata++ = get_8bit_value(p);
        		*fdata++ = get_8bit_value(p+2);
        		*fdata++ = get_8bit_value(p+4);
        	}
        }

        push_frame(0, frame);

    }
}

bool LinkyInput::set_param(const core::Parameter& param)
{
    if (assign_parameters(param)
    		(api_path_, "url")
			(key_, "key")
			(resolution_, "resolution"))
        return true;

    return core::IOThread::set_param(param);
}

} /* namespace linky_input */
} /* namespace yuri */
