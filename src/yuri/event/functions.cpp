/*!
 * @file 		functions.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		13.07.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "functions.h"
#include "EventHelpers.h"
#include <cmath>
#include <cstdlib>
namespace yuri {
namespace event {

// Helper templates
namespace {

#include "convert_to_string.impl"
#include "generic_operators.impl"

template<class Dest, class Orig, event_type_t orig_type>
pBasicEvent to_range_impl(const pBasicEvent& event, const pBasicEvent& rmin, const pBasicEvent& rmax)
{
	typedef typename Dest::stored_type type;
	typedef typename Orig::stored_type otype;
	auto ev_ptr = std::dynamic_pointer_cast<Dest>(event);
	if (!ev_ptr) throw bad_event_cast("Type mismatch in to_range_impl()");
	bool do_crop_only = false;
	if (!ev_ptr->range_specified()) do_crop_only = true;//throw bad_event_cast("Input has no range specified!()");
	if (rmin->get_type() != orig_type)
		 throw bad_event_cast("Type mismatch min value in to_range_impl()");
	if (rmax->get_type() != orig_type)
		 throw bad_event_cast("Type mismatch max value in to_range_impl()");
	otype rrmin = get_value<Orig>(rmin);
	otype rrmax = get_value<Orig>(rmax);
	if (!do_crop_only) {
		otype new_range = rrmax-rrmin;
		if (new_range <= 0) throw ("Wrong new range in to_range_impl()");
		type orig_value = ev_ptr->get_value() - ev_ptr->get_min_value();
		type orig_range = ev_ptr->get_max_value() - ev_ptr->get_min_value();
		otype new_value = static_cast<otype>(orig_value*new_range/orig_range)+rrmin;
		return std::make_shared<Orig>(new_value, rrmin, rrmax);
	} else  {
		return std::make_shared<Orig>(std::min(std::max(static_cast<otype>(ev_ptr->get_value()),rrmin),rrmax), rrmin, rrmax);
	}
}


template<class T>
pBasicEvent eq_impl2(const pBasicEvent& event0, const pBasicEvent& event1, const pBasicEvent& event2) {
	return std::make_shared<EventBool>(std::abs(get_value<T>(event0) - get_value<T>(event1)) <= get_value<T>(event2));
}

template<class T>
struct min_wrapper {
	T operator()(const T& a, const T& b) { return std::min(a,b); }
};

template<class T>
struct max_wrapper {
	T operator()(const T& a, const T& b) { return std::max(a,b); }
};
template<class T>
struct min_wrapper_vector {

};
template<class T>
struct fmod_wrapper_vector {
	T operator()(const T& a, const T& b) { return std::fmod(a,b); }
};
template<class T>
struct pow_wrapper {
	T operator()(const T& a, const T& b) { return std::pow(a,b); }
};
//template<class T, class F = std::function<typename event_traits<T>::stored_type(typename event_traits<T>::stored_type)>>
template<class T, class F = typename event_traits<T>::stored_type(*)(typename event_traits<T>::stored_type)>
pBasicEvent simple_unary_function_wrapper(const std::string& name, const std::vector<pBasicEvent>& events, F f)
{
	if (events.size() != 1) throw bad_event_cast(name + "() requires one parameter");
	return std::make_shared<T>(f(get_value<T>(events[0])));
}
}



namespace functions {
pBasicEvent str(const std::vector<pBasicEvent>& events) {
	return generic_unary_oper<convert_to_string, EventString, EventBang, EventBool, EventInt, EventDouble, EventString, EventTime, EventDuration, EventVector, EventDict>("str",events);
//	if (events.size() != 1) throw bad_event_cast("Str supports only one parameter");
//	switch (events[0]->get_type()) {
//		case event_type_t::bang_event: return convert_to_string<EventBang>(events[0]);
//		case event_type_t::integer_event: return convert_to_string<EventInt>(events[0]);
//		case event_type_t::double_event: return convert_to_string<EventDouble>(events[0]);
//		case event_type_t::string_event: return convert_to_string<EventString>(events[0]);
//		case event_type_t::boolean_event: return convert_to_string<EventBool>(events[0]);
//		case event_type_t::vector_event: return convert_to_string<EventVector>(events[0]);
//		case event_type_t::dictionary_event: return convert_to_string<EventDict>(events[0]);
//		case event_type_t::time_event: return convert_to_string<EventTime>(events[0]);
//		default: break;
//
//	}
//	throw bad_event_cast("Unsupported type of parameter in add");
}


pBasicEvent toint(const std::vector<pBasicEvent>& events) {
	if (events.size() != 1) throw bad_event_cast("int() supports only one parameter");
	switch (events[0]->get_type()) {
			case event_type_t::integer_event: return events[0];
			case event_type_t::double_event: return s_cast<EventDouble, EventInt>(events[0]);
			case event_type_t::string_event: return lex_cast<EventString, EventInt>(events[0]);
			default:break;
	}
	throw bad_event_cast("Unsupported type of parameter in int()");
}
pBasicEvent toint_range(const std::vector<pBasicEvent>& events) {
	if (events.size() != 3) throw bad_event_cast("ranged int() supports only three parameter");
	switch (events[0]->get_type()) {
			case event_type_t::integer_event: return to_range_impl<EventInt,EventInt,event_type_t::integer_event>(events[0],events[1],events[2]);
			case event_type_t::double_event: return to_range_impl<EventDouble,EventInt,event_type_t::integer_event>(events[0],events[1],events[2]);
			default:break;
	}
	throw bad_event_cast("Unsupported type of parameter in int()");
}

pBasicEvent todouble(const std::vector<pBasicEvent>& events) {
	if (events.size() != 1) throw bad_event_cast("double supports only one parameter");
	switch (events[0]->get_type()) {
			case event_type_t::integer_event: return s_cast<EventInt, EventDouble>(events[0]);
			case event_type_t::double_event: return events[0];
			case event_type_t::string_event: return lex_cast<EventString, EventDouble>(events[0]);
			default:break;
	}
	throw bad_event_cast("Unsupported type of parameter in double()");
}

pBasicEvent tobool(const std::vector<pBasicEvent>& events) {
	if (events.size() != 1) throw bad_event_cast("bool supports only one parameter");
	switch (events[0]->get_type()) {
			case event_type_t::boolean_event: return events[0];
			case event_type_t::integer_event: return s_cast<EventInt, EventBool>(events[0]);
			default:break;
	}
	throw bad_event_cast("Unsupported type of parameter in bool()");
}

pBasicEvent todouble_range(const std::vector<pBasicEvent>& events) {
	if (events.size() != 3) throw bad_event_cast("ranged double() supports only three parameter");
	switch (events[0]->get_type()) {
			case event_type_t::integer_event: return to_range_impl<EventInt,EventDouble,event_type_t::double_event>(events[0],events[1],events[2]);
			case event_type_t::double_event: return to_range_impl<EventDouble,EventDouble,event_type_t::double_event>(events[0],events[1],events[2]);

			default:break;
	}
	throw bad_event_cast("Unsupported type of parameter in double()");
}


pBasicEvent pass(const std::vector<pBasicEvent>& events)
{
	if (events.size()!=1) throw bad_event_cast("pass() supports only one parameters");
	return events[0];
}
pBasicEvent select(const std::vector<pBasicEvent>& events)
{
	if (events.size()!=2) throw bad_event_cast("select() needs 2 parameters");
	if ((events[0]->get_type() == event_type_t::integer_event) && (events[1]->get_type() == event_type_t::vector_event)) {
		int64_t index = get_value<EventInt>(events[0]);
		auto vec = std::dynamic_pointer_cast<EventVector>(events[1]);
		if (index < 0 || index >= static_cast<int64_t>(vec->size())) throw bad_event_cast("Index out of range in select()");
		return vec->at(index);
	} else if ((events[0]->get_type() == event_type_t::boolean_event) && (events[1]->get_type() == event_type_t::vector_event)) {
		size_t index = get_value<EventBool>(events[0])?0:1;
		auto vec = std::dynamic_pointer_cast<EventVector>(events[1]);
		if (index >= vec->size()) throw bad_event_cast("Index out of range in select()");
		return vec->at(index);
	} else if ((events[0]->get_type() == event_type_t::string_event) && (events[1]->get_type() == event_type_t::dictionary_event)) {
		std::string index = get_value<EventString>(events[0]);
		const auto& dict = get_value<EventDict>(events[1]);
		auto it = dict.find(index);
		if (it == dict.end()) throw bad_event_cast("Index out of range in select()");
		return it->second;
	}
	throw bad_event_cast("Wrong combination of types for select()");
}
pBasicEvent muls(const std::vector<pBasicEvent>& events)
{
	if (events.size()!=2) throw bad_event_cast("mul() needs 2 parameters");
	if ((events[0]->get_type() == event_type_t::integer_event) && (events[1]->get_type() == event_type_t::string_event)) {
		throw bad_event_cast("Wrong parameter combination to mul()");
	}
	const std::string& org = get_value<EventString>(events[0]);
	int64_t count = get_value<EventInt>(events[1]);
	std::string str;
	while (count-- > 0) str += org;
	return std::make_shared<EventString>(str);
}

pBasicEvent add(const std::vector<pBasicEvent>& events)
{
	return generic_binary_oper2<std::plus, EventInt, EventDouble, EventString, EventDuration>("add", events);
}
pBasicEvent sub(const std::vector<pBasicEvent>& events)
{
	return generic_binary_oper2<std::minus, EventInt, EventDouble, EventDuration>("sub", events);
}
pBasicEvent mul(const std::vector<pBasicEvent>& events)
{
	return generic_binary_oper2<std::multiplies, EventInt, EventDouble>("mul", events);
}
pBasicEvent div(const std::vector<pBasicEvent>& events)
{
	return generic_binary_oper2<std::divides, EventInt, EventDouble>("div", events);
}
pBasicEvent mod(const std::vector<pBasicEvent>& events)
{
	return generic_binary_oper2<std::modulus, EventInt>("mod", events);
}
pBasicEvent fmod(const std::vector<pBasicEvent>& events)
{
	return generic_binary_oper2<fmod_wrapper_vector, EventDouble>("mod", events);
}

pBasicEvent eq(const std::vector<pBasicEvent>& events) {
	if (events.size() == 2) {
		return generic_binary_oper<std::equal_to, EventBool, EventBool, EventInt, EventDouble, EventString, EventTime, EventDuration>("eq",events);
	} else if (events.size() == 3) {
		if (events[0]->get_type() != events[1]->get_type() || events[1]->get_type() != events[2]->get_type()) throw bad_event_cast("eq requires all parameters to have same type");
			switch (events[0]->get_type()) {
				case event_type_t::integer_event: return eq_impl2<EventInt>(events[0],events[1],events[2]);
				case event_type_t::double_event: return eq_impl2<EventDouble>(events[0],events[1],events[2]);
				default: throw bad_event_cast("Unsupported parameters for eq()");

			}
	}
	throw bad_event_cast("Unsupported number of parameters for eq()");
}
pBasicEvent gt(const std::vector<pBasicEvent>& events) {
	return generic_binary_oper<std::greater, EventBool, EventBool, EventInt, EventDouble, EventString, EventTime, EventDuration>("gt",events);
}
pBasicEvent ge(const std::vector<pBasicEvent>& events) {
	return generic_binary_oper<std::greater_equal, EventBool, EventBool, EventInt, EventDouble, EventString, EventTime, EventDuration>("ge",events);
}
pBasicEvent lt(const std::vector<pBasicEvent>& events) {
	return generic_binary_oper<std::less, EventBool, EventBool, EventInt, EventDouble, EventString, EventTime, EventDuration>("lt",events);
}
pBasicEvent le(const std::vector<pBasicEvent>& events) {
	return generic_binary_oper<std::less_equal, EventBool, EventBool, EventInt, EventDouble, EventString, EventTime, EventDuration>("le",events);
}

pBasicEvent	log_and(const std::vector<pBasicEvent>& events) {
	return generic_binary_oper<std::logical_and, EventBool, EventBool>("and",events);
}
pBasicEvent	log_or(const std::vector<pBasicEvent>& events) {
	return generic_binary_oper<std::logical_or, EventBool, EventBool>("or",events);
}
pBasicEvent	log_not(const std::vector<pBasicEvent>& events) {
	return generic_unary_oper<std::logical_not, EventBool, EventBool>("not",events);
}
pBasicEvent	bit_and(const std::vector<pBasicEvent>& events) {
	return generic_binary_oper<std::bit_and, EventInt, EventInt>("and",events);
}
pBasicEvent	bit_or(const std::vector<pBasicEvent>& events) {
	return generic_binary_oper<std::bit_or, EventInt, EventInt>("or",events);
}
pBasicEvent	bit_xor(const std::vector<pBasicEvent>& events) {
	return generic_binary_oper<std::bit_xor, EventInt, EventInt>("xor",events);
}

pBasicEvent	min(const std::vector<pBasicEvent>& events) {
	return generic_binary_oper2<min_wrapper, EventBool, EventInt, EventDouble, EventTime, EventString, EventDuration>("min",events);
}
pBasicEvent	max(const std::vector<pBasicEvent>& events) {
	return generic_binary_oper2<max_wrapper, EventBool, EventInt, EventDouble, EventTime, EventString, EventDuration>("max",events);
}
namespace {
template<class T>
struct abs_impl;

template<>
struct abs_impl<duration_t> {
duration_t operator()(const duration_t& val)
{
	if (val < 0_ms) return -val;
	return val;
}
};

template<class T>
struct abs_impl {
T operator()(const T& val)
{
	return std::abs(val);
}
};

}

pBasicEvent	abs(const std::vector<pBasicEvent>& events) {
	return generic_unary_oper2<abs_impl, EventInt, EventDouble, EventDuration>("abs",events);
}
typedef long double(*double_function_t)(long double);
pBasicEvent	exp(const std::vector<pBasicEvent>& events) {
	return simple_unary_function_wrapper<EventDouble>("exp", events, static_cast<double_function_t>(std::exp));
}
pBasicEvent	pow(const std::vector<pBasicEvent>& events) {
	return generic_binary_oper2<pow_wrapper, EventDouble, EventDouble>("pow",events);
}
pBasicEvent	ln(const std::vector<pBasicEvent>& events) {
	return simple_unary_function_wrapper<EventDouble>("ln",events, static_cast<double_function_t>(std::log));
}


pBasicEvent geometry(const std::vector<pBasicEvent>& events) {
	if (events.size()==1 && events[0]->get_type() == event_type_t::string_event) {
		try {
			return std::make_shared<EventString>(lexical_cast<std::string>(lex_cast_value<geometry_t>(events[0])));
		}
		catch (bad_lexical_cast&) {	}
		try {
			return std::make_shared<EventString>(lexical_cast<std::string>(lex_cast_value<resolution_t>(events[0]).get_geometry()));
		}
		catch (bad_lexical_cast&) {	}
	} else if (events.size() == 2) {
			return std::make_shared<EventString>(lexical_cast<std::string>(geometry_t{
				lex_cast_value<dimension_t>(events[0]),
				lex_cast_value<dimension_t>(events[1]),
				0,0}
				));
	} else if (events.size() == 4) {
		return std::make_shared<EventString>(lexical_cast<std::string>(
				geometry_t{lex_cast_value<dimension_t>(events[0]),
							lex_cast_value<dimension_t>(events[1]),
							lex_cast_value<position_t>(events[2]),
							lex_cast_value<position_t>(events[3])}
							));

	}
	throw bad_event_cast("Unsupported type of parameter in geometry()");
}

pBasicEvent get_width(const std::vector<pBasicEvent>& events) {
	if (events.size() != 1) throw bad_event_cast("get_width supports only one parameter");
	switch (events[0]->get_type()) {
			case event_type_t::integer_event: return events[0];

			case event_type_t::string_event: return std::make_shared<EventInt>(lex_cast_value<geometry_t>(events[0]).width);
			default:break;
	}
	throw bad_event_cast("Unsupported type of parameter in get_width()");
}
pBasicEvent get_height(const std::vector<pBasicEvent>& events) {
	if (events.size() != 1) throw bad_event_cast("get_height supports only one parameter");
	switch (events[0]->get_type()) {
			case event_type_t::integer_event: return events[0];

			case event_type_t::string_event: return std::make_shared<EventInt>(lex_cast_value<geometry_t>(events[0]).height);
			default:break;
	}
	throw bad_event_cast("Unsupported type of parameter in get_height()");
}


pBasicEvent microseconds(const std::vector<pBasicEvent>& events)
{
	if (events.size() != 1) throw bad_event_cast("microseconds supports only one parameter");
	const auto& event = events[0];
	switch (event->get_type()) {
		case event_type_t::duration_event:
			return event;
		case event_type_t::integer_event:
			return std::make_shared<EventDuration>(1_us * get_value<EventInt>(event));
		case event_type_t::double_event:
			return std::make_shared<EventDuration>(1_us * get_value<EventDouble>(event));
		default: break;

	}
	throw bad_event_cast("Unsupported type of parameter in microseconds()");
}

pBasicEvent milliseconds(const std::vector<pBasicEvent>& events)
{
	if (events.size() != 1) throw bad_event_cast("milliseconds supports only one parameter");
	const auto& event = events[0];
	switch (event->get_type()) {
		case event_type_t::duration_event:
			return event;
		case event_type_t::integer_event:
			return std::make_shared<EventDuration>(1_ms * get_value<EventInt>(event));
		case event_type_t::double_event:
			return std::make_shared<EventDuration>(1_ms * get_value<EventDouble>(event));
		default: break;

	}
	throw bad_event_cast("Unsupported type of parameter in milliseconds()");
}

pBasicEvent seconds(const std::vector<pBasicEvent>& events)
{
	if (events.size() != 1) throw bad_event_cast("seconds supports only one parameter");
	const auto& event = events[0];
	switch (event->get_type()) {
		case event_type_t::duration_event:
			return event;
		case event_type_t::integer_event:
			return std::make_shared<EventDuration>(1_s * get_value<EventInt>(event));
		case event_type_t::double_event:
			return std::make_shared<EventDuration>(1_s * get_value<EventDouble>(event));
		default: break;

	}
	throw bad_event_cast("Unsupported type of parameter in seconds()");
}

}

}

}


