/*!
 * @file 		color_events.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		10. 8. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "color.h"
#include "yuri/event/BasicEventConversions.h"
namespace yuri {
namespace core {

namespace {

using namespace yuri::event;
using core::color_t;

template <typename T, typename T2>
bool verify_range(T value, T2 min_val, T2 max_val)
{
    return (value > min_val) && (value < max_val);
}

template <typename T, int... Idx>
std::array<T, sizeof...(Idx)> create_array(const std::vector<pBasicEvent>& events)
{
    return { { lex_cast_value<T>(events[Idx])... } };
}

// template<class Event>
// pBasicEvent make_event(const pBasicEvent& ev)
//{
//	return std::make_shared<Event>(lex_cast_value<typename event_traits<Event>::stored_type>(ev));
//}

template <class Event, typename T>
pBasicEvent make_event(const T& val)
{
    return std::make_shared<Event>(lexical_cast<typename event_traits<Event>::stored_type>(val));
}

template <typename T>
pBasicEvent make_string_event(const T& val)
{
    return make_event<EventString>(val);
}

template <typename T>
pBasicEvent make_int_event(const T& val)
{
    return make_event<EventInt>(val);
}

template <typename T>
pBasicEvent make_rgb_gen(const std::vector<pBasicEvent>& events)
{
    const auto count = events.size();
    switch (count) {
    case 3:
        return make_string_event(color_t::create_rgb(create_array<T, 0, 1, 2>(events)));
    case 4:
        return make_string_event(color_t::create_rgb(create_array<T, 0, 1, 2, 3>(events)));
    default:
        throw bad_event_cast("RGB functions support only 3 or 4 parameters");
    }
}

pBasicEvent make_rgb(const std::vector<pBasicEvent>& events)
{
    return make_rgb_gen<uint8_t>(events);
}

pBasicEvent make_rgb16(const std::vector<pBasicEvent>& events)
{
    return make_rgb_gen<uint16_t>(events);
}

template <typename T>
pBasicEvent make_yuv_gen(const std::vector<pBasicEvent>& events)
{
    const auto count = events.size();
    switch (count) {
    case 3:
        return make_string_event(color_t::create_yuv(create_array<T, 0, 1, 2>(events)));
    case 4:
        return make_string_event(color_t::create_yuv(create_array<T, 0, 1, 2, 3>(events)));
    default:
        throw bad_event_cast("RGB functions support only 3 or 4 parameters");
    }
}

pBasicEvent make_yuv(const std::vector<pBasicEvent>& events)
{
    return make_yuv_gen<uint8_t>(events);
}

pBasicEvent make_yuv16(const std::vector<pBasicEvent>& events)
{
    return make_yuv_gen<uint16_t>(events);
}

pBasicEvent get_color_r(const std::vector<pBasicEvent>& events)
{
    if (events.size() != 1)
        throw bad_event_cast("get_color support only 1 parameter");

    return make_int_event(lex_cast_value<color_t>(events[0]).r());
}

pBasicEvent get_color_g(const std::vector<pBasicEvent>& events)
{
    if (events.size() != 1)
        throw bad_event_cast("get_color support only 1 parameter");

    return make_int_event(lex_cast_value<color_t>(events[0]).g());
}

pBasicEvent get_color_b(const std::vector<pBasicEvent>& events)
{
    if (events.size() != 1)
        throw bad_event_cast("get_color support only 1 parameter");

    return make_int_event(lex_cast_value<color_t>(events[0]).b());
}

pBasicEvent get_color_a(const std::vector<pBasicEvent>& events)
{
    if (events.size() != 1)
        throw bad_event_cast("get_color support only 1 parameter");

    return make_int_event(lex_cast_value<color_t>(events[0]).a());
}

pBasicEvent get_color_y(const std::vector<pBasicEvent>& events)
{
    if (events.size() != 1)
        throw bad_event_cast("get_color support only 1 parameter");

    return make_int_event(lex_cast_value<color_t>(events[0]).y());
}

pBasicEvent get_color_u(const std::vector<pBasicEvent>& events)
{
    if (events.size() != 1)
        throw bad_event_cast("get_color support only 1 parameter");

    return make_int_event(lex_cast_value<color_t>(events[0]).u());
}

pBasicEvent get_color_v(const std::vector<pBasicEvent>& events)
{
    if (events.size() != 1)
        throw bad_event_cast("get_color support only 1 parameter");

    return make_int_event(lex_cast_value<color_t>(events[0]).v());
}

FuncInitHelper fhelper_{
    { "rgb", std::vector<event_type_t>({ event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event }), event_type_t::string_event,
      make_rgb },
    { "rgb", std::vector<event_type_t>({ event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event }),
      event_type_t::string_event, make_rgb },
    { "rgba", std::vector<event_type_t>({ event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event }),
      event_type_t::string_event, make_rgb },

    { "rgb16", std::vector<event_type_t>({ event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event }), event_type_t::string_event,
      make_rgb16 },
    { "rgb16",
      std::vector<event_type_t>({ event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event }),
      event_type_t::string_event, make_rgb16 },
    { "rgba16",
      std::vector<event_type_t>({ event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event }),
      event_type_t::string_event, make_rgb16 },

    { "yuv", std::vector<event_type_t>({ event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event }), event_type_t::string_event,
      make_yuv },
    { "yuv", std::vector<event_type_t>({ event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event }),
      event_type_t::string_event, make_yuv },
    { "yuva", std::vector<event_type_t>({ event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event }),
      event_type_t::string_event, make_yuv },

    { "yuv16", std::vector<event_type_t>({ event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event }), event_type_t::string_event,
      make_yuv16 },
    { "yuv16",
      std::vector<event_type_t>({ event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event }),
      event_type_t::string_event, make_yuv16 },
    { "yuva16",
      std::vector<event_type_t>({ event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event }),
      event_type_t::string_event, make_yuv16 },

    { "color_r", std::vector<event_type_t>({ event_type_t::string_event }), event_type_t::integer_event, get_color_r },
    { "color_g", std::vector<event_type_t>({ event_type_t::string_event }), event_type_t::integer_event, get_color_g },
    { "color_b", std::vector<event_type_t>({ event_type_t::string_event }), event_type_t::integer_event, get_color_b },
    { "color_a", std::vector<event_type_t>({ event_type_t::string_event }), event_type_t::integer_event, get_color_a },
    { "color_y", std::vector<event_type_t>({ event_type_t::string_event }), event_type_t::integer_event, get_color_y },
    { "color_u", std::vector<event_type_t>({ event_type_t::string_event }), event_type_t::integer_event, get_color_u },
    { "color_v", std::vector<event_type_t>({ event_type_t::string_event }), event_type_t::integer_event, get_color_v },
};
}
}
}
