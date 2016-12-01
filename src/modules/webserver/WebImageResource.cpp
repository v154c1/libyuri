/*!
 * @file 		WebImageResource.cpp
 * @author 		Zdenek Travnicek <travnicek@cesnet.cz>
 * @date		07.12.2014
 * @copyright	CESNET, z.s.p.o, 2014
 * 				Distributed under modified BSD or GPL License,
 * 				see /doc/LICENSE.txt for details
 *
 */
#include "WebImageResource.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/compressed_frame_types.h"
#include "yuri/core/frame/compressed_frame_params.h"
namespace yuri {
namespace webserver {

IOTHREAD_GENERATOR(WebImageResource)

core::Parameters WebImageResource::configure()
{
    core::Parameters p = base_type::configure();
    p.set_description("WebImageResource");
    p["server_name"]["Name of server"] = "webserver";
    p["path"]["Name of server"]        = "/image";
    return p;
}

WebImageResource::WebImageResource(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters)
    : base_type(log_, parent, std::string("web_static")),
      WebResource(log_),
      server_name_("webserver"),
      path_("/image"),
      rnd_generator_(random_device_()),
      distribution_(0, 1000000UL)
{
    IOTHREAD_INIT(parameters)
    set_supported_formats({ core::compressed_frame::jpeg, core::compressed_frame::png });
}

WebImageResource::~WebImageResource() noexcept
{
}

void WebImageResource::run()
{
    while (still_running() && !register_to_server(server_name_, path_, std::dynamic_pointer_cast<WebResource>(get_this_ptr()))) {
        sleep(10_ms);
    }
    log[log::info] << "Registered to server";
    base_type::run();
}
core::pFrame WebImageResource::do_special_single_step(core::pCompressedVideoFrame frame)
{
    {
        std::unique_lock<std::mutex> _(frame_lock_);
        last_frame_ = frame;
        etag_       = distribution_(rnd_generator_);
    }
    return frame;
}

webserver::response_t WebImageResource::do_process_request(const webserver::request_t& request)
{
    core::pCompressedVideoFrame frame;
    auto                        it       = request.parameters.find("If-None-Match");
    uint64_t                    req_etag = (it != request.parameters.end()) ? lexical_cast<uint64_t>(it->second) : 0UL;

    {
        std::unique_lock<std::mutex> _(frame_lock_);
        if (req_etag == etag_) {
            log[log::info] << "Returning 304, server already hs the latest image";
        }
        frame = last_frame_;
    }

    if (!frame)
        throw std::runtime_error("Image not available yet");
    log[log::info] << "Responding";
    const auto&       fi   = core::compressed_frame::get_format_info(frame->get_format());
    const std::string mime = fi.mime_types.empty() ? "image/jpeg" : fi.mime_types[0];
    return response_t{ http_code::ok,
                       { { "Content-Encoding", mime },
                         { "Etag", std::to_string(etag_) },
                         { "Cache-Control", "must-revalidate, no-cache" }, //, no-store, must-revalidate"},
                         { "Pragma", "no-cache" },
                         { "Expires", "0" } },
                       std::string(frame->begin(), frame->end()) };
}
bool WebImageResource::set_param(const core::Parameter& param)
{
    if (assign_parameters(param)      //
        (server_name_, "server_name") //
        (path_, "path")) {
        return true;
    }
    return base_type::set_param(param);
}

} /* namespace web_static */
} /* namespace yuri */
