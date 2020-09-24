/*!
 * @file 		ExtrapolateEvents.h
 * @author 		Zdenek Travnicek <v154c1@gmail.com>
 * @date 		18.08.2020
 * @copyright	Institute of Intermedia, CTU in Prague, 2020
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef EXTRAPOLATEEVENTS_H_
#define EXTRAPOLATEEVENTS_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/event/BasicEventConsumer.h"
#include "yuri/event/BasicEventProducer.h"
#include "EventRateLimiter.h"
#include <chrono>

namespace yuri {
    namespace extrapolate_events {
        struct event_info_t {
            // Last received value
            double last_value;
            // Speed in units/ns. If NaN, we don't have any speed info yet
            double current_speed;
            // Time the last event was received
            std::chrono::high_resolution_clock::time_point last_update;
        };

        class ExtrapolateEvents
                : public  event_rate::EventRateLimiter<event_info_t> {
        public:
            IOTHREAD_GENERATOR_DECLARATION

            static core::Parameters configure();

            ExtrapolateEvents(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);

            virtual ~ExtrapolateEvents() noexcept;

        private:

            bool set_param(const core::Parameter &param) override;

            bool do_process_event(const std::string &event_name, const event::pBasicEvent &event);

            void output_event(const std::string &event_name, event_info_t &data,
                              std::chrono::high_resolution_clock::time_point now) override;

        private:

        };

    } /* namespace extrapolate_events */
} /* namespace yuri */
#endif /* EXTRAPOLATEEVENTS_H_ */
