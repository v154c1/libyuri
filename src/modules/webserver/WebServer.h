/*!
 * @file 		WebServer.h
 * @author 		Zdenek Travnicek <travnicek@cesnet.cz>
 * @date		01.12.2014
 * @copyright	CESNET, z.s.p.o, 2014
 * 				Distributed under modified BSD or GPL License,
 * 				see /doc/LICENSE.txt for details
 *
 */

#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/core/socket/StreamSocket.h"
#include "common_types.h"
#include "web_exceptions.h"
#include <future>
#include <deque>
#include <condition_variable>
#include <tuple>
namespace yuri {
namespace webserver {

struct client_info_t {
    core::socket::pStreamSocket socket_;
    std::future<response_t>     response_;
};

class WebServer;
using pWebServer  = std::shared_ptr<WebServer>;
using pwWebServer = std::weak_ptr<WebServer>;

class WebResource;
using pWebResource  = std::shared_ptr<WebResource>;
using pwWebResource = std::weak_ptr<WebResource>;

pwWebServer find_webserver(const std::string& name);

struct route_record {
    std::string  routing_spec;
    pWebResource resource;
};

using f_request_t = std::future<bool>;

class WebServer : public core::IOThread {
public:
    IOTHREAD_GENERATOR_DECLARATION
    static core::Parameters configure();
    WebServer(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters);
    virtual ~WebServer() noexcept;
    bool register_resource(const std::string& routing_spec, pWebResource);

private:
    virtual void run() override;
    virtual bool set_param(const core::Parameter& param) override;

    request_t read_request(core::socket::pStreamSocket socket);
    bool reply_to_client(core::socket::pStreamSocket& socket, response_t response);
    response_t auth_response(request_t request);
    response_t find_response(request_t request);

    void response_thread();
    void push_request(f_request_t request);
    f_request_t pop_request();
    bool        authentication_needed();
    bool        verify_authentication(const request_t&);

    bool process_request(core::socket::pStreamSocket client);
    std::string server_name_;
    std::string socket_impl_;
    std::string address_;
    uint16_t    port_;
    std::string realm_;
    std::string user_;
    std::string pass_;
    bool        cors_;

    core::socket::pStreamSocket socket_;
    std::vector<route_record>   routing_;
    std::mutex                  routing_mutex_;

    std::deque<f_request_t> requests_;
    std::mutex              request_mutex_;

    std::condition_variable request_notify_;
};

} /* namespace webserver */
} /* namespace yuri */
#endif /* WEBSERVER_H_ */
