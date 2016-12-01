/*!
 * @file 		WebDirectoryResource.h
 * @author 		Zdenek Travnicek <travnicek@cesnet.cz>
 * @date		15.12.2014
 * @copyright	CESNET, z.s.p.o, 2014
 * 				Distributed under modified BSD or GPL License,
 * 				see /doc/LICENSE.txt for details
 *
 */

#ifndef SRC_MODULES_WEBSERVER_WEBDIRECTORYRESOURCE_H_
#define SRC_MODULES_WEBSERVER_WEBDIRECTORYRESOURCE_H_

#include "yuri/core/thread/IOThread.h"
#include "WebResource.h"

namespace yuri {
namespace webserver {

class WebDirectoryResource : public core::IOThread, public WebResource {
public:
    IOTHREAD_GENERATOR_DECLARATION
    static core::Parameters configure();
    WebDirectoryResource(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters);
    virtual ~WebDirectoryResource() noexcept;

private:
    virtual void run() override;
    virtual bool set_param(const core::Parameter& param) override;
    virtual webserver::response_t do_process_request(const webserver::request_t& request) override;
    std::string server_name_;
    std::string path_;
    std::string directory_;
    std::string index_file_;
};

} /* namespace webserver  */
} /* namespace yuri */

#endif /* SRC_MODULES_WEBSERVER_WEBDIRECTORYRESOURCE_H_ */
