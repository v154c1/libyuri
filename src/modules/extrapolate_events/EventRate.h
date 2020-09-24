/*!
 * @file 		EventRate.h
 * @author 		Zdenek Travnicek <v154c1@gmail.com>
 * @date 		18.08.2020
 * @copyright	Institute of Intermedia, CTU in Prague, 2020
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef EVENTRATE_H_
#define EVENTRATE_H_

#include "yuri/core/thread/IOThread.h"
#include "EventRateLimiter.h"

namespace yuri {
    namespace event_rate {

        class EventRate : public event_rate::EventRateLimiter<event::pBasicEvent> {
        public:
            IOTHREAD_GENERATOR_DECLARATION

            static core::Parameters configure();

            EventRate(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);

            virtual ~EventRate() noexcept = default;

        private:


            virtual bool set_param(const core::Parameter &param) override;

            bool do_process_event(const std::string &event_name, const event::pBasicEvent &event);

            void output_event(const std::string &event_name, event::pBasicEvent &data,
                              std::chrono::high_resolution_clock::time_point now) override;
        };

    } /* namespace event_rate */
} /* namespace yuri */
#endif /* EVENTRATE_H_ */
