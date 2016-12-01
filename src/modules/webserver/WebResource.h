/*!
 * @file 		WebResource.h
 * @author 		Zdenek Travnicek <travnicek@cesnet.cz>
 * @date		02.12.2014
 * @copyright	CESNET, z.s.p.o, 2014
 * 				Distributed under modified BSD or GPL License,
 * 				see /doc/LICENSE.txt for details
 *
 */

#ifndef WEBRESOURCE_H_
#define WEBRESOURCE_H_

#include "yuri/core/thread/IOThread.h"
#include "WebServer.h"

namespace yuri {
namespace webserver {

class WebResource {
public:
    WebResource(const log::Log& log_);
    virtual ~WebResource() noexcept;
    webserver::response_t process_request(const webserver::request_t& request);

protected:
    bool register_to_server(const std::string& server_name, const std::string& routing_spec, pWebResource resource);

private:
    log::Log                      log_res;
    bool                          registered_;
    virtual webserver::response_t do_process_request(const webserver::request_t& request) = 0;
};

} /* namespace webresource */
} /* namespace yuri */
#endif /* WEBRESOURCE_H_ */
