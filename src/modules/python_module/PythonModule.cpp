/*!
 * @file 		PythonModule.cpp
 * @author 		Zdenek Travnicek <v154c1@gmail.com>
 * @date 		06.09.2023
 * @copyright	Institute of Intermedia, CTU in Prague, 2023
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "PythonModule.h"
#include "yuri/core/Module.h"
#include "yuri/version.h"
#include "yuri/core/utils/irange.h"
#include "yuri/core/utils.h"
#include "yuri/event/EventHelpers.h"

namespace yuri {
    namespace python_module {

        std::mutex PythonModule::instance_mutex_;


        IOTHREAD_GENERATOR(PythonModule)

        MODULE_REGISTRATION_BEGIN("python_module")
            REGISTER_IOTHREAD("python", PythonModule)
        MODULE_REGISTRATION_END()

        core::Parameters PythonModule::configure() {
            core::Parameters p = core::IOThread::configure();
            p.set_description("PythonModule");
            p["file"]["Path to a script file"] = "";
            p["script"]["Script (in a CDATA section)"] = "";
            return p;
        }

        namespace {
            template<class EventType>
            auto define_basic_event(const char *name) {
                using StoredType = typename event::event_traits<EventType>::stored_type;
                using getter_type = StoredType(*)(const EventType &);
                return boost::python::class_<EventType, std::shared_ptr<EventType>, boost::noncopyable>
                        (name, boost::python::no_init)
                        .add_property
                                ("value",
                                 boost::python::make_function<getter_type>(
                                         [](const EventType &event) -> StoredType { return event.get_value(); }));

            }

            template<class EventType>
            auto define_ranged_event(const char *name) {
                using StoredType = typename event::event_traits<EventType>::stored_type;
                using getter_type = StoredType(*)(const EventType &);
                return define_basic_event<EventType>(name)
                        .add_property("min_value", boost::python::make_function<getter_type>(
                                [](const EventType &event) { return event.get_min_value(); }))
                        .add_property("max_value", boost::python::make_function<getter_type>(
                                [](const EventType &event) -> StoredType { return event.get_max_value(); }));
            }


        }

        bool dispatch_emit(PythonModule &module, const std::string &name, boost::python::object obj) {

            if (obj.is_none()) {
                return module.emit_event(name);
            }
            const std::string type_name = boost::python::extract<std::string>(
                    obj.attr("__class__").attr("__name__"));
            if (type_name == "bool") {
                return module.emit_event(name, static_cast<bool>(boost::python::extract<bool>(obj)));
            } else if (type_name == "str") {
                return module.emit_event(name, boost::python::extract<std::string>(obj));
            } else if (type_name == "int") {
                return module.emit_event(name, static_cast<int64_t>(boost::python::extract<int64_t>(obj)));
            } else if (type_name == "float") {
                return module.emit_event(name, static_cast<double>(boost::python::extract<double>(obj)));
            }

            return false;

        }

        boost::python::dict unwrap_dict(const PythonModule &module) {
            if (module.events_) {
                return *module.events_;
            }
            return {};
        }

        BOOST_PYTHON_MODULE (yuri) {
            boost::python::class_<PythonModule, std::shared_ptr<PythonModule>, boost::noncopyable>("Yuri",
                                                                                                   boost::python::no_init)
                    .def("info", &PythonModule::info, boost::python::args("text"))
                    .add_property("version", &PythonModule::version)
                    .def < bool(PythonModule::*)(const std::string&)>
            ("emit", &PythonModule::emit_event, boost::python::args("name"))

                    .def("emit", &dispatch_emit, boost::python::args("name", "value"))

                    .def("register_change", &PythonModule::register_change_handler)
                    .def("register_tick", &PythonModule::register_tick_handler)
                    .add_property("events", &unwrap_dict);

            define_basic_event<event::EventBool>("EventBool");
            define_ranged_event<event::EventInt>("EventInt");
            define_ranged_event<event::EventDouble>("EventDouble");
            define_basic_event<event::EventString>("EventString");
        }

        namespace {
            void init_python() {
                static bool first = true;
                if (first) {
                    PyImport_AppendInittab("yuri", &PyInit_yuri);
                    Py_Initialize();
                }
                first = false;

            }
        }

        PythonModule::PythonModule(
                const log::Log &log_, core::pwThreadBase
        parent,
                const core::Parameters &parameters)
                :
                core::IOThread(log_, parent, 1, 1, std::string("python_module")),
                event::BasicEventConsumer(log), event::BasicEventProducer(log) {
            IOTHREAD_INIT(parameters)
        }

        PythonModule::~PythonModule()
        noexcept {
        }

        bool PythonModule::set_param(const core::Parameter &param) {
            if (assign_parameters(param)
                    (script_text_, "script")
                    (script_file_, "file")) {
                return true;
            }
            return core::IOThread::set_param(param);
        }

        void PythonModule::run() {
            namespace python = boost::python;
            try {
                std::unique_lock<std::mutex> lock(instance_mutex_);
                init_python();
                events_ = std::make_shared<boost::python::dict>();
                python::dict main_ns;
                main_ns["__builtins__"] = python::import("builtins");
                main_ns["yuri"] = python::import("yuri");

                python::exec("def onchange(args=[]):\n"
                             "    def wrapper(func):\n"
                             "        y.register_change(func, args)\n"
                             "        return func\n"
                             "    return wrapper\n"
                             "\n"
                             "def ontick(func):\n"
                             "    # def wrapper(func):\n"
                             "    y.register_tick(func)\n"
                             "    return func", main_ns, main_ns);

                main_ns["y"] = std::dynamic_pointer_cast<PythonModule>(shared_from_this());
                if (!script_text_.empty()) {
                    python::exec(script_text_.c_str(), main_ns, main_ns);
                }
                if (!script_file_.empty()) {
                    python::exec_file(script_file_.c_str(), main_ns, main_ns);
                }
                while (still_running()) {
                    wait_for_events(10_ms);
                    process_events();
                    for (auto &tick: tick_callbacks_) {
                        try {
                            tick();
                        }
                        catch (const python::error_already_set &) {
                            handle_python_error();
                        }
                    }

                }
            }
            catch (const python::error_already_set &) {
                handle_python_error();

            }
        }

        void PythonModule::handle_python_error() {
            PyObject * ptype = nullptr, *pvalue = nullptr, *ptraceback = nullptr;
            PyErr_Fetch(&ptype, &pvalue, &ptraceback);

            PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
            if (ptraceback != nullptr) {
                PyException_SetTraceback(pvalue, ptraceback);
            }

            boost::python::handle<> htype(ptype);
            boost::python::handle<> hvalue(boost::python::allow_null(pvalue));
            boost::python::handle<> htraceback(boost::python::allow_null(ptraceback));
            boost::python::api::object traceback = boost::python::import("traceback");
            boost::python::api::object format_exception = traceback.attr("format_exception");
            boost::python::api::object formatted_list = format_exception(htype, hvalue, htraceback);
            boost::python::api::object formatted = boost::python::str("\n").join(formatted_list);
            std::string s = boost::python::extract<std::string>(formatted);
            log[log::error] << s;
        }

        namespace {
            boost::python::object get_object(const event::pBasicEvent &event) {
                switch (event->get_type()) {
                    case event::event_type_t::integer_event:
                        return boost::python::object{
                                std::dynamic_pointer_cast<event::EventInt>(event)};

                    case event::event_type_t::double_event:
                        return boost::python::object{
                                std::dynamic_pointer_cast<event::EventDouble>(event)};
                    case event::event_type_t::boolean_event:
                        return boost::python::object{
                                std::dynamic_pointer_cast<event::EventBool>(event)};

                    case event::event_type_t::string_event:
                        return boost::python::object{
                                std::dynamic_pointer_cast<event::EventString>(event)};
                    default:
                        return {};

                }
            }
        }

        bool PythonModule::do_process_event(const std::string &event_name, const event::pBasicEvent &event) {
            try {
                auto obj = get_object(event);
                if (events_ && !obj.is_none()) {

                    (*events_)[event_name] = obj;
                }

                for (const auto &cb: callbacks_) {
                    if (cb.events.empty() || contains(cb.events, event_name)) {
                        try {
                            cb.obj(event_name, obj);
                        }
                        catch (const boost::python::error_already_set &) {
                            handle_python_error();
                        }
                    }
                }

            }

            catch (const boost::python::error_already_set &) {
                handle_python_error();
                return false;
            }

            return true;
        }

        void PythonModule::info(const std::string &msg) {
            log[log::info] << msg;
        }

        std::string PythonModule::version() const {
            return yuri_version;
        }

        void PythonModule::register_change_handler(boost::python::object obj, const boost::python::list &events) {
            log[log::info] << "Register method";
            callback_info_t info{{}, std::move(obj)};
            const auto size = boost::python::len(events);
            info.events.resize(size);
            for (const auto &i: irange(size)) {
                info.events[i] = boost::python::extract<std::string>(events[i]);
            }
            callbacks_.emplace_back(std::move(info));

        }

        void PythonModule::register_tick_handler(boost::python::object obj) {
            log[log::info] << "Register tick";
            tick_callbacks_.emplace_back(std::move(obj));
        }

    } /* namespace python_module */
} /* namespace yuri */
