/*!
 * @file 		WebServer.h
 * @author 		<Your name>
 * @date 		01.12.2014
 * @copyright	Institute of Intermedia, 2013
 * 				Distributed BSD License
 *
 */

#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/core/socket/StreamSocket.h"
#include "common_types.h"
#include <future>
#include <deque>
#include <condition_variable>
namespace yuri {
namespace webserver {



struct client_info_t {
	core::socket::pStreamSocket socket_;
	std::future<response_t> response_;
};

class WebServer;
using pWebServer = std::shared_ptr<WebServer>;
using pwWebServer = std::weak_ptr<WebServer>;

class WebResource;
using pWebResource = std::shared_ptr<WebResource>;
using pwWebResource= std::weak_ptr<WebResource>;

pwWebServer find_webserver(const std::string& name);

struct route_record {
	std::string routing_spec;
	pWebResource resource;
};

using f_request_t = std::future<request_t>;

class WebServer: public core::IOThread
{
public:

	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	WebServer(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~WebServer() noexcept;
	bool register_resource (const std::string& routing_spec, pWebResource);
private:
	
	virtual void run();
	virtual bool set_param(const core::Parameter& param);

	request_t read_request(core::socket::pStreamSocket socket);
	bool reply_to_client(core::socket::pStreamSocket& socket, response_t response);
	response_t find_response(request_t request);

	void response_thread();
	void push_request(f_request_t request);
	f_request_t pop_request();

	std::string server_name_;
	std::string socket_impl_;
	std::string address_;
	uint16_t port_;

	core::socket::pStreamSocket socket_;
	std::vector<route_record> routing_;

	std::deque<f_request_t> requests_;
	std::mutex request_mutex_;

	std::condition_variable request_notify_;

};

} /* namespace webserver */
} /* namespace yuri */
#endif /* WEBSERVER_H_ */
