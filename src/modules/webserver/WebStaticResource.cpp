/*!
 * @file 		WebStaticResource.cpp
 * @author 		Zdenek Travnicek <travnicek@cesnet.cz>
 * @date		02.12.2014
 * @copyright	CESNET, z.s.p.o, 2014
 * 				Distributed under modified BSD or GPL License,
 * 				see /doc/LICENSE.txt for details
 *
 */

#include "WebStaticResource.h"
#include "yuri/core/Module.h"
#include <fstream>
namespace yuri {
namespace webserver {

IOTHREAD_GENERATOR(WebStaticResource)

core::Parameters WebStaticResource::configure()
{
    core::Parameters p = core::IOThread::configure();
    p.set_description("WebStaticResource");
    p["server_name"]["Name of server"]    = "webserver";
    p["path"]["Name of server"]           = "/image";
    p["mime"]["image/jpeg"]               = "image/jpeg";
    p["filename"]["Path to the resource"] = "";
    return p;
}

WebStaticResource::WebStaticResource(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters)
    : core::IOThread(log_, parent, 1, 1, std::string("web_static")), WebResource(log_), server_name_("webserver"), path_("/image"), mime_type_("image/jpeg")
{
    IOTHREAD_INIT(parameters)
    std::ifstream file(filename_, std::ios::in | std::ios::binary);
    file.seekg(0, std::ios::end);
    auto len = file.tellg();
    log[log::info] << "Resizing to " << len;
    data_string_.resize(len);
    file.seekg(0, std::ios::beg);
    file.read(&data_string_[0], len);
}

WebStaticResource::~WebStaticResource() noexcept
{
}

void WebStaticResource::run()
{
    while (still_running() && !register_to_server(server_name_, path_, std::dynamic_pointer_cast<WebResource>(get_this_ptr()))) {
        sleep(10_ms);
    }
    log[log::info] << "Registered to server";
    while (still_running()) {
        sleep(100_ms);
    }
}

webserver::response_t WebStaticResource::do_process_request(const webserver::request_t& /*request*/)
{
    log[log::info] << "Responding";
    return response_t{ http_code::ok, { { "Content-Encoding", mime_type_ } }, data_string_ };
}
bool WebStaticResource::set_param(const core::Parameter& param)
{
    if (assign_parameters(param)      //
        (server_name_, "server_name") //
        (path_, "path")               //
        (mime_type_, "mime")          //
        (filename_, "filename")) {
        return true;
    }

    return core::IOThread::set_param(param);
}

} /* namespace web_static */
} /* namespace yuri */
