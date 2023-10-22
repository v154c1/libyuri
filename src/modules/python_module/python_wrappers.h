//
// Created by neneko on 10.9.23.
//

#ifndef YURI2_PYTHON_WRAPPERS_H
#define YURI2_PYTHON_WRAPPERS_H

#include "boost/python.hpp"
#include "yuri/event/EventHelpers.h"

namespace yuri {
    namespace python_module {
//        namespace {
//            template<class EventType>
//            auto define_basic_event(const char *name) {
//                using StoredType = typename event::event_traits<EventType>::stored_type;
//                using getter_type = StoredType(*)(const EventType &);
//                return boost::python::class_<EventType, std::shared_ptr<EventType>, boost::noncopyable>
//                                                                                    (name, boost::python::no_init)
//                                                                                            .add_property
//                                                                                                    ("value",
//                                                                                                     boost::python::make_function<getter_type>(
//                                                                                                             [](const EventType &event) -> StoredType { return event.get_value(); }));
//
//            }
//
//            template<class EventType>
//            auto define_ranged_event(const char *name) {
//                using StoredType = typename event::event_traits<EventType>::stored_type;
//                using getter_type = StoredType(*)(const EventType &);
//                return define_basic_event<EventType>(name)
//                        .add_property("min_value", boost::python::make_function<getter_type>(
//                                [](const EventType &event) { return event.get_min_value(); }))
//                        .add_property("max_value", boost::python::make_function<getter_type>(
//                                [](const EventType &event) -> StoredType { return event.get_max_value(); }));
//            }
//
//
//        }
    }
}
#endif //YURI2_PYTHON_WRAPPERS_H
