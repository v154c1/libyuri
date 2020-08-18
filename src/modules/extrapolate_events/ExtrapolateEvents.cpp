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

        MODULE_REGISTRATION_BEGIN("extrapolate_events")
            REGISTER_IOTHREAD("extrapolate_events", ExtrapolateEvents)
        MODULE_REGISTRATION_END()

        core::Parameters ExtrapolateEvents::configure() {
            core::Parameters p = core::IOThread::configure();
            p.set_description("Extrapolates event values and outputs them in regular intervals.");
            p["fps"]["Output framerate"] = 25;
            return p;
        }


        ExtrapolateEvents::ExtrapolateEvents(const log::Log &log_, core::pwThreadBase parent,
                                             const core::Parameters &parameters) :
                core::IOThread(log_, parent, 0, 0, std::string("extrapolate_events")),
                event::BasicEventConsumer(log),
                event::BasicEventProducer(log),
                output_counter_(0) {
            IOTHREAD_INIT(parameters)
            set_latency(0.1_ms);
            output_start_ = std::chrono::high_resolution_clock::now();
        }

        ExtrapolateEvents::~ExtrapolateEvents() noexcept = default;


        bool ExtrapolateEvents::set_param(const core::Parameter &param) {
            if (assign_parameters(param)
                    (fps_, "fps")) {
                return true;
            }
            return core::IOThread::set_param(param);
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

        void ExtrapolateEvents::run() {
            auto next_update = output_start_;
            while (still_running()) {
                wait_for_events(get_latency());
                process_events();
                const auto now = std::chrono::high_resolution_clock::now();
                if (now >= next_update) {
                    for (const auto &ev: events_) {
                        const auto& info = ev.second;
                        if (std::isnan(info.current_speed)) {
                            continue;
                        }
                        const auto current_value = std::chrono::duration_cast<std::chrono::nanoseconds>(now - info.last_update).count() * info.current_speed + info.last_value;
                        emit_event(ev.first, current_value);
                    }

                    next_update = output_start_ + std::chrono::microseconds(++output_counter_ * 1_s / fps_);
                }
            }
        }

    } /* namespace extrapolate_events */
} /* namespace yuri */
