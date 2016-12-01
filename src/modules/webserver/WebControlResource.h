/*
 * WebControlResource.h
 *
 *  Created on: 14. 12. 2014
 *      Author: neneko
 */

#ifndef SRC_MODULES_WEBSERVER_WEBCONTROLRESOURCE_H_
#define SRC_MODULES_WEBSERVER_WEBCONTROLRESOURCE_H_

#include "yuri/core/thread/IOThread.h"
#include "WebResource.h"
#include "yuri/event/BasicEventProducer.h"
namespace yuri {
namespace webserver {

class WebControlResource : public core::IOThread, public WebResource, public event::BasicEventProducer {
public:
    IOTHREAD_GENERATOR_DECLARATION
    static core::Parameters configure();
    WebControlResource(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters);
    virtual ~WebControlResource() noexcept;

private:
    virtual void run() override;
    virtual bool set_param(const core::Parameter& param) override;
    virtual webserver::response_t do_process_request(const webserver::request_t& request) override;
    std::string server_name_;
    std::string path_;
    std::string redirect_path_;
};
}
}

#endif /* SRC_MODULES_WEBSERVER_WEBCONTROLRESOURCE_H_ */
