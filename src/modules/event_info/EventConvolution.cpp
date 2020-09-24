/*!
 * @file 		EventConvolution.cpp
 * @author 		Zdenek Travnicek <v154c1@gmail.com>
 * @date 		18.08.2020
 * @copyright	Institute of Intermedia, CTU in Prague, 2020
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include <numeric>
#include "EventConvolution.h"
#include "yuri/core/Module.h"
#include "yuri/core/utils/string.h"

namespace yuri {
    namespace event_convolution {

        IOTHREAD_GENERATOR(EventConvolution)

        core::Parameters EventConvolution::configure() {
            core::Parameters p = core::IOThread::configure();
            p.set_description(
                    "Multiplies event value (and previous values) with a convolution kernel. The resulted value is then weighted with a previous output.");
            p["weight"]["Weight for the new value"] = 0.75;
            p["kernel"]["Vector of values for the kernel (either a vector of doubles or space separated list of values"] = "1";
            return p;
        }


        EventConvolution::EventConvolution(const log::Log &log_, core::pwThreadBase parent,
                                           const core::Parameters &parameters) :
                core::IOThread(log_, parent, 0, 0, std::string("event_convolution")),
                event::BasicEventConsumer(log),
                event::BasicEventProducer(log) {
            IOTHREAD_INIT(parameters)
            {
                auto l = log[log::info];
                l << "Kernel: ";
                for (const auto &i: kernel_) {
                    l << i << ' ';
                }
            }
        }

        EventConvolution::~EventConvolution() noexcept {
        }

        void EventConvolution::run() {
            while (still_running()) {
                wait_for_events(get_latency());
                process_events();
            }
        }

        bool EventConvolution::set_param(const core::Parameter &param) {
            if (assign_parameters(param)
                    (weight_, "weight")
                    (kernel_, "kernel", [&](const core::Parameter &p) {
                        std::vector<double> out;
                        if (const auto &value = std::dynamic_pointer_cast<event::EventVector>(p.get_value())) {
                            out.resize(value->size());
                            std::transform(value->cbegin(), value->cend(), out.begin(),
                                           [](const event::pBasicEvent &ev) {
                                               return event::lex_cast_value<double>(ev);
                                           });
                        } else {
                            const auto &text = event::lex_cast_value<std::string>(p.get_value());
                            auto as_text = core::utils::split_string(text, ' ');
                            out.resize(as_text.size());
                            std::transform(as_text.cbegin(), as_text.cend(), out.begin(), [](const std::string &num) {
                                return std::stod(num);
                            });
                        }
                        return out;
                    })) {
                return true;
            }


            return core::IOThread::set_param(param);
        }

        bool EventConvolution::do_process_event(const std::string &event_name, const event::pBasicEvent &event) {
            if (!values_.count(event_name)) {
                values_[event_name] = {};
                last_values_[event_name] = 0.0;
            }
            auto &vals = values_[event_name];
            vals.push_front(event::lex_cast_value<double>(event));
            while (vals.size() > kernel_.size()) {
                vals.pop_back();
            }
            log[log::info] << "already have " << vals.size();
            if (vals.size() == kernel_.size()) {
                const auto val = std::inner_product(vals.cbegin(), vals.cend(), kernel_.cbegin(), 0.0);
                const auto next_value = val * weight_ + last_values_.at(event_name) * (1.0 - weight_);
                emit_event(event_name, next_value);
                last_values_[event_name] = next_value;
            }

            return false;
        }
    } /* namespace event_convolution */
} /* namespace yuri */
