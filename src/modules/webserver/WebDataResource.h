//
// Created by neneko on 11/3/19.
//

#ifndef YURI2_WEBDATRESOURCE_H
#define YURI2_WEBDATRESOURCE_H

#include "yuri/core/thread/IOThread.h"
#include "WebResource.h"
#include "yuri/event/BasicEventProducer.h"
#include <map>
#include <mutex>

namespace yuri {
    namespace webserver {

        class WebDataResource : public core::IOThread, public WebResource, public event::BasicEventConsumer {
        public:
            IOTHREAD_GENERATOR_DECLARATION

            static core::Parameters configure();

            WebDataResource(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);

            ~WebDataResource();

        private:
            void run();

            virtual bool set_param(const core::Parameter &param) override;

            virtual bool do_process_event(const std::string &event_name, const event::pBasicEvent &event);

            virtual webserver::response_t do_process_request(const webserver::request_t& request);
            std::string server_name_;
            std::string path_;

            std::map<std::string, event::pBasicEvent> events_;
            std::mutex event_lock_;
        };

    }
}

#endif //YURI2_WEBDATRESOURCE_H
