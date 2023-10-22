/*!
 * @file 		PythonModule.h
 * @author 		Zdenek Travnicek <v154c1@gmail.com>
 * @date 		06.09.2023
 * @copyright	Institute of Intermedia, CTU in Prague, 2023
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef PYTHONMODULE_H_
#define PYTHONMODULE_H_
#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <boost/python.hpp>

#include "yuri/core/thread/IOThread.h"
#include "yuri/event/BasicEventConsumer.h"
#include "yuri/event/BasicEventProducer.h"

namespace yuri {
    namespace python_module {
//struct  wrapper_t;
//        void init_module_yuri();

        class PythonModule : public core::IOThread, public event::BasicEventConsumer, public event::BasicEventProducer {
//    friend struct wrapper_t;
            friend void init_module_yuri();

            friend boost::python::dict unwrap_dict(const PythonModule &module);

            friend bool dispatch_emit(PythonModule &module, const std::string& name, boost::python::object obj);

        public:
            IOTHREAD_GENERATOR_DECLARATION

            static core::Parameters configure();

            PythonModule(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);

            virtual ~PythonModule() noexcept;

        private:
            bool do_process_event(const std::string &event_name, const event::pBasicEvent &event) override;

        public:
            void info(const std::string &msg);

            std::string version() const;

        protected:
            void run() override;

            void register_change_handler(boost::python::object, const boost::python::list &events);

            void register_tick_handler(boost::python::object);

        private:
            struct callback_info_t {
                std::vector<std::string> events;
                boost::python::object obj;
            };


            virtual bool set_param(const core::Parameter &param) override;

            std::vector<callback_info_t> callbacks_;
//            std::vector<std::string> event_names_;
            std::shared_ptr<boost::python::dict> events_;
            std::vector<boost::python::object> tick_callbacks_;
            std::string script_file_;
            std::string script_text_;

            void handle_python_error();
        };

    } /* namespace python_module */
} /* namespace yuri */
#endif /* PYTHONMODULE_H_ */
