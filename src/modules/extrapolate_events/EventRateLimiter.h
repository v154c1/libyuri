//
// Created by neneko on 18.08.20.
//

#ifndef YURI2_EVENTRATELIMITER_H
#define YURI2_EVENTRATELIMITER_H

#include "yuri/core/thread/IOThread.h"
#include "yuri/event/BasicEventConsumer.h"
#include "yuri/event/BasicEventProducer.h"
#include <chrono>
#include <yuri/core/utils/assign_parameters.h>
#include <cmath>

namespace yuri {
    namespace event_rate {

        template<class EventInfo>
        class EventRateLimiter
                : public core::IOThread, public event::BasicEventConsumer, public event::BasicEventProducer {
        public:
            IOTHREAD_GENERATOR_DECLARATION

            static core::Parameters configure() {
                core::Parameters p = core::IOThread::configure();
                p.set_description("Generic event rate limiter.");
                p["fps"]["Output framerate"] = 25;
                return p;
            }

            EventRateLimiter(const log::Log &log_, core::pwThreadBase parent, const core::Parameters & /* parameters */)
                    : core::IOThread(log_, parent, 0, 0, std::string("extrapolate_events")),
                      event::BasicEventConsumer(log),
                      event::BasicEventProducer(log),
                      output_counter_(0) {
                set_latency(0.1_ms);
                output_start_ = std::chrono::high_resolution_clock::now();
            }

            virtual ~EventRateLimiter() noexcept = default;

            bool set_param(const core::Parameter &param) override {
                if (assign_parameters(param)
                        (fps_, "fps")) {
                    return true;
                }
                return core::IOThread::set_param(param);
            }

        private:

            void run() override {
                auto next_update = output_start_;
                while (still_running()) {
                    wait_for_events(get_latency());
                    process_events();
                    const auto now = std::chrono::high_resolution_clock::now();
                    if (now >= next_update) {
                        for (auto &ev: events_) {
                            auto &info = ev.second;
                            output_event(ev.first, info, now);
                        }

                        next_update = output_start_ + std::chrono::microseconds(++output_counter_ * 1_s / fps_);
                    }
                }
            }

            virtual void output_event(const std::string &event_name, EventInfo &data,
                                      std::chrono::high_resolution_clock::time_point now) = 0;

//            virtual bool do_process_event(const std::string &event_name, const event::pBasicEvent &event) {
//                return false;
//            }

        private:
            double fps_;

            std::chrono::high_resolution_clock::time_point output_start_;
            int64_t output_counter_;
        protected:
            std::map<std::string, EventInfo> events_;
        };

    }
}


#endif //YURI2_EVENTRATELIMITER_H
