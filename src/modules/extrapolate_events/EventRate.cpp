/*!
 * @file 		EventRate.cpp
 * @author 		Zdenek Travnicek <v154c1@gmail.com>
 * @date 		18.08.2020
 * @copyright	Institute of Intermedia, CTU in Prague, 2020
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "EventRate.h"
#include "yuri/core/Module.h"

namespace yuri {
    namespace event_rate {


        IOTHREAD_GENERATOR(EventRate)

        core::Parameters EventRate::configure() {
            core::Parameters p = core::IOThread::configure();
            p.set_description("Limits and fixed event rate ");
            return p;
        }


        EventRate::EventRate(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters) :
                EventRateLimiter(log_, parent, parameters) {
            IOTHREAD_INIT(parameters)
        }


        bool EventRate::set_param(const core::Parameter &param) {
            return EventRateLimiter::set_param(param);
        }

        bool EventRate::do_process_event(const std::string &event_name, const event::pBasicEvent &event) {
            this->events_[event_name] = event;
            return true;
        }

        void EventRate::output_event(const std::string &event_name, event::pBasicEvent &data,
                                     std::chrono::high_resolution_clock::time_point /* now */) {
            emit_event(event_name, data);
        }

    } /* namespace event_rate */
} /* namespace yuri */
