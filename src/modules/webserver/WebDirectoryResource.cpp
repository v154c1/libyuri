/*!
 * @file 		WebDirectoryResource.cpp
 * @author 		Zdenek Travnicek <travnicek@cesnet.cz>
 * @date		15.12.2014
 * @copyright	CESNET, z.s.p.o, 2014
 * 				Distributed under modified BSD or GPL License,
 * 				see /doc/LICENSE.txt for details
 *
 */

#include <modules/webserver/WebDirectoryResource.h>
#include "yuri/core/Module.h"
#include "yuri/core/utils.h"
#include <fstream>
namespace yuri {
namespace webserver {

IOTHREAD_GENERATOR(WebDirectoryResource)

core::Parameters WebDirectoryResource::configure()
{
    core::Parameters p = core::IOThread::configure();
    p.set_description("WebDirectoryResource");
    p["server_name"]["Name of server"]                               = "webserver";
    p["path"]["prefix for the served resource"]                      = "/";
    p["dir"]["Path to the resource"]                                 = ".";
    p["index_file"]["Nme of index file when directory is requested"] = "index.html";
    return p;
}

WebDirectoryResource::WebDirectoryResource(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters)
    : core::IOThread(log_, parent, 1, 1, std::string("web_static")), WebResource(log_), server_name_("webserver"), path_("/image"), directory_(".")
{
    IOTHREAD_INIT(parameters)
}

WebDirectoryResource::~WebDirectoryResource() noexcept
{
}

void WebDirectoryResource::run()
{
    while (still_running() && !register_to_server(server_name_, path_ + ".*", std::dynamic_pointer_cast<WebResource>(get_this_ptr()))) {
        sleep(10_ms);
    }
    log[log::info] << "Registered to server";
    while (still_running()) {
        sleep(100_ms);
    }
}

namespace {

std::string get_filename(const std::string& path)
{
    auto idx = path.find_last_of('/');
    return path.substr(idx + 1, std::string::npos);
}

std::string get_extension(const std::string filename)
{
    auto idx = filename.find_last_of('.');
    return filename.substr(idx + 1, std::string::npos);
}

const std::map<std::string, std::string> mime_types{
    { "html", "text/html" },     //
    { "htm", "text/html" },      //
    { "txt", "text/plain" },     //
    { "xml", "text/xml" },       //
    { "css", "text/css" },       //
    { "js", "text/javascript" }, //
    { "jpg", "image/jpeg" },     //
    { "jpeg", "image/jpeg" },    //
    { "png", "image/png" },      //
    { "gif", "image/gif" },      //
    { "webp", "image/webp" },
};

const std::string& default_mime_type = "text/plain";
const std::string& guess_mime_type(const std::string& path)
{
    auto ext = get_extension(get_filename(path));
    for (auto& c : ext) {
        c = std::tolower(c);
    }
    auto it = mime_types.find(ext);
    if (it == mime_types.end()) {
        return default_mime_type;
    }
    return it->second;
}
}

webserver::response_t WebDirectoryResource::do_process_request(const webserver::request_t& request)
{
    log[log::info] << "Responding";
    const auto& path = request.url.path;
    // Some basic precaution from malicious requests
    if (path.find("..") != path.npos) {
        throw not_found(path);
    }
    auto       suffix = path.substr(path_.size(), path.npos);
    const bool is_dir = suffix.empty() || (suffix.back() == '/');
    if (is_dir) {
        suffix += index_file_;
    }
    const auto filename = directory_ + suffix;
    try {
        std::ifstream file(filename, std::ios::in | std::ios::binary);
        file.seekg(0, std::ios::end);
        auto len = file.tellg();
        if (len < 0) {
            throw not_found(path);
        }
        log[log::info] << "preparing " << len << " bytes";
        std::string data_string(len, 0);
        file.seekg(0, std::ios::beg);
        file.read(&data_string[0], len);

        return response_t{ http_code::ok, { { "Content-Encoding", guess_mime_type(suffix) } }, data_string };
    } catch (std::runtime_error&) {
        log[log::warning] << "Failed to process " << filename;
        throw not_found(path);
    }
}
bool WebDirectoryResource::set_param(const core::Parameter& param)
{
    if (assign_parameters(param)      //
        (server_name_, "server_name") //
        (path_, "path")               //
        (directory_, "directory")     //
        (index_file_, "index_file")) {
        if (path_.empty() || path_[path_.size() - 1] != '/') {
            path_ += "/";
        }
        if (directory_.empty() || directory_[directory_.size() - 1] != '/') {
            directory_ += "/";
        }
        return true;
    }

    return core::IOThread::set_param(param);
}

} /* namespace webserver */
} /* namespace yuri */
