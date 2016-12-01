/*!
 * @file 		WebResource.cpp
 * @author 		Zdenek Travnicek <travnicek@cesnet.cz>
 * @date		02.12.2014
 * @copyright	CESNET, z.s.p.o, 2014
 * 				Distributed under modified BSD or GPL License,
 * 				see /doc/LICENSE.txt for details
 *
 */

#include "WebResource.h"
#include "yuri/core/Module.h"

namespace yuri {
namespace webserver {

WebResource::WebResource(const log::Log& log_) : log_res(log_), registered_(false)
{
}

WebResource::~WebResource() noexcept
{
}

bool WebResource::register_to_server(const std::string& server_name, const std::string& routing_spec, pWebResource resource)
{
    if (registered_)
        return true;
    auto server_ptr = webserver::find_webserver(server_name);
    if (!server_ptr.expired()) {
        auto server = server_ptr.lock();
        return (registered_ = server->register_resource(routing_spec, resource));
    }
    return false;
}

webserver::response_t WebResource::process_request(const webserver::request_t& request)
{
    return do_process_request(request);
}

} /* namespace webresource */
} /* namespace yuri */
