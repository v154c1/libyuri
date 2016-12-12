/*!
 * @file 		WebServer.cpp
 * @author 		Zdenek Travnicek <travnicek@cesnet.cz>
 * @date		01.12.2014
 * @copyright	CESNET, z.s.p.o, 2014
 * 				Distributed under modified BSD or GPL License,
 * 				see /doc/LICENSE.txt for details
 *
 */

#include "WebServer.h"
#include "WebResource.h"
#include "WebPageGenerator.h"
#include "base64.h"
#include "yuri/core/Module.h"
#include "yuri/core/socket/StreamSocketGenerator.h"
#include "yuri/version.h"
#include <boost/regex.hpp>

namespace yuri {
namespace webserver {

IOTHREAD_GENERATOR(WebServer)

core::Parameters WebServer::configure()
{
    core::Parameters p = core::IOThread::configure();
    p.set_description("WebServer");
    p["socket"]["Socket implementation"]                                            = "yuri_tcp";
    p["address"]["Server address"]                                                  = "0.0.0.0";
    p["server_name"]["Server name"]                                                 = "webserver";
    p["port"]["Server port"]                                                        = 8080;
    p["realm"]["Realm for HTTP authentication"]                                     = "";
    p["username"]["Username for HTTP authentication"]                               = "";
    p["password"]["Password for HTTP authentication"]                               = "";
    p["cors"]["Disable CORS (adds Access-Control-Allow-Origin header:* when true)"] = true;
    return p;
}

namespace {
const std::string crlf = "\r\n";

std::map<std::string, pwWebServer> active_servers;
std::mutex active_servers_mutex;
void register_server(const std::string& name, pwWebServer server)
{
    std::unique_lock<std::mutex> _(active_servers_mutex);
    active_servers[name] = server;
}
}

pwWebServer find_webserver(const std::string& name)
{
    std::unique_lock<std::mutex> _(active_servers_mutex);
    auto                         it = active_servers.find(name);
    if (it == active_servers.end())
        return {};
    return it->second;
}

WebServer::WebServer(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters)
    : core::IOThread(log_, parent, 1, 1, std::string("webserver")),
      server_name_("webserver"),
      socket_impl_("yuri_tcp"),
      address_("0.0.0.0"),
      port_(8080),
      cors_(true)
{
    IOTHREAD_INIT(parameters)
    socket_ = core::StreamSocketGenerator::get_instance().generate(socket_impl_, log);
    log[log::info] << "Created socket";
    if (!socket_->bind(address_.c_str(), port_)) {
        log[log::fatal] << "Failed to bind to " + address_ + ":" + std::to_string(port_);
        throw exception::InitializationFailed("Failed to bind to port " + address_ + ":" + std::to_string(port_));
    }
    if (!socket_->listen()) {
        log[log::fatal] << "Failed to start listening";
        throw exception::InitializationFailed("Failed to start listening");
    }
}

WebServer::~WebServer() noexcept
{
}

void WebServer::run()
{
    register_server(server_name_, std::dynamic_pointer_cast<WebServer>(get_this_ptr()));
    log[log::info] << "Starting worker thread";
    auto req_thread = std::thread{ [&]() { response_thread(); } };

    while (still_running()) {
        if (socket_->wait_for_data(get_latency())) {
            auto client = socket_->accept();
            log[log::info] << "Connection accepted";
            push_request(std::async(std::launch::async, [=]() mutable { return process_request(client); }));
        }
    }
    log[log::info] << "Joining worker thread";
    req_thread.join();
}

response_t WebServer::auth_response(request_t request)
{
    if (authentication_needed()) {
        if (!verify_authentication(request)) {
            auto response                           = get_default_response(http_code::unauthorized);
            response.parameters["WWW-Authenticate"] = "Basic realm=\"" + realm_ + "\"";
            return response;
        }
    }
    return find_response(request);
}

response_t WebServer::find_response(request_t request)
{
    std::vector<route_record> viable_routes;
    {
        std::unique_lock<std::mutex> _(routing_mutex_);
        for (const auto& route : routing_) {
            boost::regex url(route.routing_spec);
            if (boost::regex_match(request.url.path.cbegin(), request.url.path.cend(), url)) {
                viable_routes.push_back(route);
            }
        }
    }
    for (const auto& route : viable_routes) {
        try {
            return route.resource->process_request(request);
        } catch (redirect_to& redirect) {
            log[log::info] << "Redirecting to " << redirect.get_location();
            return get_redirect_response(redirect.get_code(), redirect.get_location());
        } catch (not_found&) {
            // This isn't fatal, there may be another resource publishing this path.
            continue;
        } catch (not_modified& /* redirect */) {
            //				log[log::info] << "Returning 304 for resource";
            return response_t{ http_code::not_modified, parameters_t{}, {} };

        } catch (std::runtime_error& e) {
            const std::string msg = e.what();
            log[log::info] << "Returning 500 for URL " << request.url.path << " (" << msg << ")";
            return get_default_response(http_code::server_error, msg);
        }
    }
    log[log::info] << "Returning 404 for URL " << request.url.path;
    return get_default_response(http_code::not_found);
}

void WebServer::response_thread()
{
    log[log::info] << "Helper thread started";
    while (still_running()) {
        auto fr = pop_request();
        if (fr.valid()) {
            try {
                auto status = fr.wait_for(std::chrono::microseconds(get_latency()));
                if (status != std::future_status::ready) {
                    push_request(std::move(fr));
                } else {
                    auto res = fr.get();
                    if (!res) {
                        log[log::warning] << "Failed to process connection";
                    }
                }
            } catch (std::runtime_error& e) {
                log[log::warning] << "Failed to process connection (" << e.what() << ")";
            }
        }
    }
    log[log::info] << "Helper thread ending";
}

namespace {
inline void fill_header_if_needed(response_t& response, const std::string& name, const std::string& value)
{
    auto it = response.parameters.find(name);
    if (it == response.parameters.end()) {
        response.parameters[name] = value;
    }
}
}

bool WebServer::process_request(core::socket::pStreamSocket client)
{
    auto request = read_request(client);
    log[log::info] << "Requested URL: " << request.url.path;

    response_t response = auth_response(request);
    if (request.method == "HEAD") {
        // For HEAD request, there should be no body, but the Content-Length should be set
        fill_header_if_needed(response, "Content-Length", std::to_string(response.data.size()));
        response.data.clear();
    }
    reply_to_client(request.client, std::move(response));
    return true;
}

void WebServer::push_request(f_request_t request)
{
    std::unique_lock<std::mutex> _(request_mutex_);
    requests_.push_back(std::move(request));
    // This could be sub-optimal, but it makes the logic easier...
    request_notify_.notify_all();
}
f_request_t WebServer::pop_request()
{
    std::unique_lock<std::mutex> lock(request_mutex_);
    // Don't wait for variable, if there's data ready...
    if (!requests_.empty()) {
        auto r = std::move(requests_.front());
        requests_.pop_front();
        return r;
    }
    // No data available, so wait for a notification or timeout
    request_notify_.wait_for(lock, std::chrono::microseconds(get_latency()));
    if (!requests_.empty()) {
        auto r = std::move(requests_.front());
        requests_.pop_front();
        return r;
    }
    // No data even after timeout, nothing to return
    return {};
}

namespace {
bool data_finished(const std::string& data)
{
    const auto len = data.size();
    if (len < 4)
        return false;
    if (data[len - 4] == '\r' && data[len - 3] == '\n' && data[len - 2] == '\r' && data[len - 1] == '\n')
        return true;
    // Let's support event \n\n
    if (data[len - 2] == '\n' && data[len - 1] == '\n')
        return true;
    return false;
}
}
request_t WebServer::read_request(core::socket::pStreamSocket client)
{
    request_t         request{ {}, parameters_t{}, {}, client };
    std::vector<char> data(0);
    data.resize(1024);
    std::string request_string;

    while (!data_finished(request_string) && still_running()) {
        if (client->wait_for_data(get_latency())) {
            auto read = client->receive_data(data);
            if (!read)
                throw std::runtime_error("Failed to read data");
            request_string.append(data.begin(), data.begin() + read);
        }
    }

    boost::regex url_line("^([A-Z]+) (.*) HTTP/1.[01]\r?\n");

    boost::smatch what;
    auto          start = request_string.cbegin();
    const auto    end   = request_string.cend();
    if (regex_search(start, end, what, url_line, boost::match_default)) {
        request.method = what[1].str();
        request.url    = parse_url(std::string(what[2].first, what[2].second));
        start          = what[0].second;
        boost::regex           param_line("([^:]+):([^\r\n]*)\r?\n");
        boost::sregex_iterator i(start, end, param_line, boost::match_default);
        boost::sregex_iterator j;
        while (i != j) {
            const auto& res                = *i;
            const auto  param_name         = std::string(res[1].first, res[1].second);
            request.parameters[param_name] = std::string(res[2].first, res[2].second);
            ++i;
        }
    } else {
        log[log::warning] << "Failed request: " << request_string;
        throw std::runtime_error("Failed to parse url");
    }

    return request;
}

bool WebServer::reply_to_client(core::socket::pStreamSocket& client, response_t response)
{
    fill_header_if_needed(response, "Content-Length", std::to_string(response.data.size()));
    fill_header_if_needed(response, "Server", std::string("yuri-") + yuri_version);
    if (cors_) {
        fill_header_if_needed(response, "Access-Control-Allow-Origin", "*");
    }
    client->send_data(prepare_response_header(response.code));
    client->send_data(crlf);
    for (const auto& param : response.parameters) {
        client->send_data(param.first);
        client->send_data(std::string(": "));
        client->send_data(param.second);
        client->send_data(crlf);
    }
    client->send_data(crlf);
    client->send_data(response.data);
    return true;
}

bool WebServer::register_resource(const std::string& routing_spec, pWebResource resource)
{
    std::unique_lock<std::mutex> _(routing_mutex_);
    routing_.push_back({ routing_spec, std::move(resource) });
    return true;
}

bool WebServer::authentication_needed()
{
    return !realm_.empty();
}
bool WebServer::verify_authentication(const request_t& request)
{
    auto it = request.parameters.find("Authorization");
    if (it == request.parameters.end())
        return false;
    boost::regex  auth_line("Basic ([a-zA-Z0-9+/=]+)");
    boost::smatch what;
    if (regex_search(it->second.cbegin(), it->second.cend(), what, auth_line, boost::match_default)) {
        const auto auth_str = std::string(what[1].first, what[1].second);
        const auto decoded  = base64::decode(auth_str);
        auto       idx      = decoded.find(':');
        const auto name     = decoded.substr(0, idx);
        const auto pass     = decoded.substr(idx + 1);
        //		log[log::info] << "User: " << name << ", pass: " << pass;
        if (!user_.empty()) {
            if (user_ != name) {
                log[log::warning] << "Wrong username!";
                return false;
            }
        }
        if (!pass_.empty()) {
            if (pass_ != pass) {
                log[log::warning] << "Wrong password!";
                return false;
            }
        }
        if (user_.empty()) {
            log[log::info] << "Authenticated anonymous user";
        } else {
            log[log::info] << "Authenticated user " + user_;
        }
        return true;
    }
    log[log::warning] << "Failed to parse AUthentication header";
    return false;
}

bool WebServer::set_param(const core::Parameter& param)
{
    if (assign_parameters(param) //
        (socket_impl_, "socket") //
        (address_, "address")    //
        (port_, "port")          //
        (user_, "username")      //
        (pass_, "password")      //
        (realm_, "realm")        //
        (cors_, "cors")) {
        return true;
    }

    return core::IOThread::set_param(param);
}

} /* namespace webserver */
} /* namespace yuri */
