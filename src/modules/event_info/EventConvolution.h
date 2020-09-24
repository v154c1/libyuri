/*!
 * @file 		EventConvolution.h
 * @author 		Zdenek Travnicek <v154c1@gmail.com>
 * @date 		18.08.2020
 * @copyright	Institute of Intermedia, CTU in Prague, 2020
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef EVENTCONVOLUTION_H_
#define EVENTCONVOLUTION_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/event/BasicEventConsumer.h"
#include "yuri/event/BasicEventProducer.h"

namespace yuri {
    namespace event_convolution {

        class EventConvolution
                : public core::IOThread, public event::BasicEventConsumer, public event::BasicEventProducer {
        public:
            IOTHREAD_GENERATOR_DECLARATION

            static core::Parameters configure();

            EventConvolution(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);

            virtual ~EventConvolution() noexcept;

        private:

            virtual void run() override;

            virtual bool set_param(const core::Parameter &param) override;

            bool do_process_event(const std::string &event_name, const event::pBasicEvent &event) override;

            std::vector<double> kernel_;
            std::map<std::string, std::deque<double>> values_;
            std::map<std::string, double> last_values_;
            double weight_;
        };

    } /* namespace event_convolution */
} /* namespace yuri */
#endif /* EVENTCONVOLUTION_H_ */
