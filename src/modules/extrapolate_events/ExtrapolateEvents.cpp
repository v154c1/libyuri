/*!
 * @file 		ExtrapolateEvents.cpp
 * @author 		Zdenek Travnicek <v154c1@gmail.com>
 * @date 		18.08.2020
 * @copyright	Institute of Intermedia, CTU in Prague, 2020
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "ExtrapolateEvents.h"
#include "yuri/core/Module.h"
#include <cmath>

namespace yuri {
    namespace extrapolate_events {


        IOTHREAD_GENERATOR(ExtrapolateEvents)

        core::Parameters ExtrapolateEvents::configure() {
            core::Parameters p = EventRateLimiter::configure();
            p.set_description("Extrapolates event values and outputs them in regular intervals.");
            return p;
        }


        ExtrapolateEvents::ExtrapolateEvents(const log::Log &log_, core::pwThreadBase parent,
                                             const core::Parameters &parameters) :
                EventRateLimiter(log_, parent, parameters) {
            IOTHREAD_INIT(parameters)
        }

        ExtrapolateEvents::~ExtrapolateEvents() noexcept = default;


        bool ExtrapolateEvents::set_param(const core::Parameter &param) {
            return EventRateLimiter::set_param(param);
        }

        bool ExtrapolateEvents::do_process_event(const std::string &event_name, const event::pBasicEvent &event) {
            if (event->get_type() != event::event_type_t::integer_event &&
                event->get_type() != event::event_type_t::double_event) {
                log[log::warning] << "Unsupported event type for event " << event_name;
            }
            const auto value = event::lex_cast_value<double>(event);
            const auto now = std::chrono::high_resolution_clock::now();
            if (!events_.count(event_name)) {
                // Event received for the first time
                events_[event_name] = {
                        value,
                        std::numeric_limits<double>::quiet_NaN(),
                        now
                };
            } else {
                auto &info = events_.at(event_name);
                const auto delta_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        now - info.last_update).count();
                info.current_speed = (value - info.last_value) / delta_ns;
                info.last_value = value;
                info.last_update = now;
                log[log::debug] << "Current speed for " << event_name << " is " << info.current_speed << "/ns";
            }

            return false;
        }

        void ExtrapolateEvents::output_event(const std::string &event_name, event_info_t &info,
                                             std::chrono::high_resolution_clock::time_point now) {

            if (std::isnan(info.current_speed)) {
                return;
            }
            const auto current_value =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(now - info.last_update).count() *
                    info.current_speed + info.last_value;
            emit_event(event_name, current_value);
        }

    } /* namespace extrapolate_events */
} /* namespace yuri */
