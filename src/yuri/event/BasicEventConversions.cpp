/*!
 * @file 		BasicEventConversions.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		04.07.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "BasicEventConversions.h"
#include "functions.h"
#include <iostream>
#include <cassert>
namespace yuri {
namespace event {

void EventFunctions::add_function(const event_function_record_t& frec)
{
	lock_t _(map_mutex_);
	functions_.insert({frec.fname,frec});
}
void EventFunctions::add_functions(const std::vector<event_function_record_t>& records)
{
	for (const auto& f: records) add_function(f);
}

/**
 *
 * @param fname
 * @param events
 * @return
 *
 * @bug NOT really thread-safe...
 */
pBasicEvent call(const std::string& fname, const std::vector<pBasicEvent>& events)
{
	const auto& functions = EventFunctionsFactory::get_instance().get_map();
	const auto& iters = functions.equal_range(fname);
	if (iters.first==functions.end()) throw bad_event_cast("Unknown function");
	// Looking for exact param match
	std::vector<const event_function_record_t*> candidates;
	for (auto it=iters.first;it!=iters.second;++it) {
		const auto& ptypes = it->second.param_types;
		bool ok = true;
		if (ptypes.size() != events.size()) continue;
		candidates.push_back(&it->second);
		for (size_t i=0;i<events.size();++i) {
			assert(events[i]);
			if (ptypes[i] != events[i]->get_type()) {
				ok = false;
				break;
			}
		}
		if (!ok) continue;
		return it->second.func_(events);
	}
	// No exact match found, we should try implicit conversions.
	for (const auto& f: candidates) {
		const auto& ptypes = f->param_types;
		std::vector<pBasicEvent> converted_events;
		bool ok = true;
		for (size_t i=0;i<events.size();++i) {
			if (ptypes[i] == events[i]->get_type()) {
				converted_events.push_back(events[i]);
			} else {
				try {
					auto p = implicit_conversion(events[i], ptypes[i]);
					converted_events.push_back(p);
				}
				catch (bad_event_cast& ) {
					ok = false;
					break;
				}
			}
		}
		if (!ok) continue;
		return f->func_(converted_events);
	}
	throw bad_event_cast("Unsupported parameter combination");
}


pBasicEvent implicit_conversion(pBasicEvent event, event_type_t target_type)
{
	// Implicit conversions are hard-coded!
	auto conv = std::make_pair(event->get_type(), target_type);
	/*if (cmp_event_pair(conv, {event_type_t::double_event, event_type_t::integer_event})) {
		return s_cast<EventDouble, EventInt>(event);
	} else */if (cmp_event_pair(conv, {event_type_t::integer_event, event_type_t::double_event})) {
		return s_cast<EventInt, EventDouble>(event);
	} else if (cmp_event_pair(conv, {event_type_t::integer_event, event_type_t::boolean_event})) {
		return s_cast<EventDouble, EventBool>(event);
	} else{
		throw bad_event_cast("Implicit cast failed");
	}
}





namespace {

FuncInitHelper fhelper_ {

		 {"str", std::vector<event_type_t>({event_type_t::bang_event}),event_type_t::string_event,functions::str},
		 {"str", std::vector<event_type_t>({event_type_t::boolean_event}),event_type_t::string_event,functions::str},
		 {"str", std::vector<event_type_t>({event_type_t::integer_event}),event_type_t::string_event,functions::str},
		 {"str", std::vector<event_type_t>({event_type_t::double_event}),event_type_t::string_event,functions::str},
		 {"str", std::vector<event_type_t>({event_type_t::string_event}),event_type_t::string_event,functions::str},
		 {"str", std::vector<event_type_t>({event_type_t::vector_event}),event_type_t::string_event,functions::str},
		 {"str", std::vector<event_type_t>({event_type_t::dictionary_event}),event_type_t::string_event,functions::str},
		 {"str", std::vector<event_type_t>({event_type_t::duration_event}),event_type_t::string_event,functions::str},

		 {"double", std::vector<event_type_t>({event_type_t::integer_event}),event_type_t::double_event, functions::todouble},
		 {"double", std::vector<event_type_t>({event_type_t::double_event}),event_type_t::double_event, functions::todouble},
		 {"double", std::vector<event_type_t>({event_type_t::string_event}),event_type_t::double_event, functions::todouble},

		 {"double", std::vector<event_type_t>({event_type_t::double_event, event_type_t::double_event, event_type_t::double_event}),event_type_t::double_event, functions::todouble_range},
		 {"double", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::double_event, event_type_t::double_event}),event_type_t::double_event, functions::todouble_range},

		 {"int", std::vector<event_type_t>({event_type_t::double_event}),event_type_t::integer_event, functions::toint},
		 {"int", std::vector<event_type_t>({event_type_t::integer_event}),event_type_t::integer_event, functions::toint},
		 {"int", std::vector<event_type_t>({event_type_t::string_event}),event_type_t::integer_event, functions::toint},

		 {"int", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event}),event_type_t::integer_event, functions::toint_range},
		 {"int", std::vector<event_type_t>({event_type_t::double_event, event_type_t::integer_event, event_type_t::integer_event}),event_type_t::integer_event, functions::toint_range},

		 {"bool", std::vector<event_type_t>({event_type_t::boolean_event}),event_type_t::boolean_event, functions::tobool},
		 {"bool", std::vector<event_type_t>({event_type_t::integer_event}),event_type_t::boolean_event, functions::tobool},

		 {"pass", std::vector<event_type_t>({event_type_t::bang_event}),event_type_t::bang_event, functions::pass},
		 {"pass", std::vector<event_type_t>({event_type_t::boolean_event}),event_type_t::boolean_event, functions::pass},
		 {"pass", std::vector<event_type_t>({event_type_t::integer_event}),event_type_t::integer_event, functions::pass},
		 {"pass", std::vector<event_type_t>({event_type_t::double_event}),event_type_t::double_event, functions::pass},
		 {"pass", std::vector<event_type_t>({event_type_t::string_event}),event_type_t::string_event, functions::pass},
		 {"pass", std::vector<event_type_t>({event_type_t::time_event}),event_type_t::time_event, functions::pass},
		 {"pass", std::vector<event_type_t>({event_type_t::vector_event}),event_type_t::vector_event, functions::pass},
		 {"pass", std::vector<event_type_t>({event_type_t::dictionary_event}),event_type_t::dictionary_event, functions::pass},

		 {"select", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::vector_event}),event_type_t::undetermined_event, functions::select},
		 {"select", std::vector<event_type_t>({event_type_t::boolean_event, event_type_t::vector_event}),event_type_t::undetermined_event, functions::select},
		 {"select", std::vector<event_type_t>({event_type_t::string_event, event_type_t::dictionary_event}),event_type_t::undetermined_event, functions::select},

		 {"add", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::integer_event}),event_type_t::integer_event,functions::add},
		 {"add", std::vector<event_type_t>({event_type_t::double_event, event_type_t::double_event}),event_type_t::double_event,functions::add},
		 {"add", std::vector<event_type_t>({event_type_t::string_event, event_type_t::string_event}),event_type_t::string_event,functions::add},
		 {"add", std::vector<event_type_t>({event_type_t::duration_event, event_type_t::duration_event}),event_type_t::duration_event,functions::add},

		 {"sub", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::integer_event}),event_type_t::integer_event,functions::sub},
		 {"sub", std::vector<event_type_t>({event_type_t::double_event, event_type_t::double_event}),event_type_t::double_event,functions::sub},
		 {"sub", std::vector<event_type_t>({event_type_t::duration_event, event_type_t::duration_event}),event_type_t::duration_event,functions::sub},

		 {"mul", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::integer_event}),event_type_t::integer_event,functions::mul},
		 {"mul", std::vector<event_type_t>({event_type_t::double_event, event_type_t::double_event}),event_type_t::double_event,functions::mul},
		 {"mul", std::vector<event_type_t>({event_type_t::string_event, event_type_t::integer_event}),event_type_t::string_event,functions::muls},

		 {"div", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::integer_event}),event_type_t::integer_event,functions::div},
		 {"div", std::vector<event_type_t>({event_type_t::double_event, event_type_t::double_event}),event_type_t::double_event,functions::div},

		 {"mod", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::integer_event}),event_type_t::integer_event,functions::mod},
		 {"mod", std::vector<event_type_t>({event_type_t::double_event, event_type_t::double_event}),event_type_t::double_event,functions::fmod},


		 {"eq", std::vector<event_type_t>({event_type_t::boolean_event, event_type_t::boolean_event}),event_type_t::boolean_event, functions::eq},
		 {"eq", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::integer_event}),event_type_t::boolean_event, functions::eq},
		 {"eq", std::vector<event_type_t>({event_type_t::double_event, event_type_t::double_event}),event_type_t::boolean_event, functions::eq},
		 {"eq", std::vector<event_type_t>({event_type_t::string_event, event_type_t::string_event}),event_type_t::boolean_event, functions::eq},
		 {"eq", std::vector<event_type_t>({event_type_t::time_event, event_type_t::time_event}),event_type_t::boolean_event, functions::eq},
		 {"eq", std::vector<event_type_t>({event_type_t::duration_event, event_type_t::duration_event}),event_type_t::boolean_event, functions::eq},
		 {"eq", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::integer_event, event_type_t::integer_event}),event_type_t::boolean_event, functions::eq},
		 {"eq", std::vector<event_type_t>({event_type_t::double_event, event_type_t::double_event, event_type_t::double_event}),event_type_t::boolean_event, functions::eq},


		 {"gt", std::vector<event_type_t>({event_type_t::boolean_event, event_type_t::boolean_event}),event_type_t::boolean_event, functions::gt},
		 {"gt", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::integer_event}),event_type_t::boolean_event, functions::gt},
		 {"gt", std::vector<event_type_t>({event_type_t::double_event, event_type_t::double_event}),event_type_t::boolean_event, functions::gt},
		 {"gt", std::vector<event_type_t>({event_type_t::string_event, event_type_t::string_event}),event_type_t::boolean_event, functions::gt},
		 {"gt", std::vector<event_type_t>({event_type_t::time_event, event_type_t::time_event}),event_type_t::boolean_event, functions::gt},
		 {"gt", std::vector<event_type_t>({event_type_t::duration_event, event_type_t::duration_event}),event_type_t::boolean_event, functions::gt},

		 {"ge", std::vector<event_type_t>({event_type_t::boolean_event, event_type_t::boolean_event}),event_type_t::boolean_event, functions::ge},
		 {"ge", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::integer_event}),event_type_t::boolean_event, functions::ge},
		 {"ge", std::vector<event_type_t>({event_type_t::double_event, event_type_t::double_event}),event_type_t::boolean_event, functions::ge},
		 {"ge", std::vector<event_type_t>({event_type_t::string_event, event_type_t::string_event}),event_type_t::boolean_event, functions::ge},
		 {"ge", std::vector<event_type_t>({event_type_t::time_event, event_type_t::time_event}),event_type_t::boolean_event, functions::ge},
		 {"ge", std::vector<event_type_t>({event_type_t::duration_event, event_type_t::duration_event}),event_type_t::boolean_event, functions::ge},

		 {"lt", std::vector<event_type_t>({event_type_t::boolean_event, event_type_t::boolean_event}),event_type_t::boolean_event, functions::lt},
		 {"lt", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::integer_event}),event_type_t::boolean_event, functions::lt},
		 {"lt", std::vector<event_type_t>({event_type_t::double_event, event_type_t::double_event}),event_type_t::boolean_event, functions::lt},
		 {"lt", std::vector<event_type_t>({event_type_t::string_event, event_type_t::string_event}),event_type_t::boolean_event, functions::lt},
		 {"lt", std::vector<event_type_t>({event_type_t::time_event, event_type_t::time_event}),event_type_t::boolean_event, functions::lt},
		 {"lt", std::vector<event_type_t>({event_type_t::duration_event, event_type_t::duration_event}),event_type_t::boolean_event, functions::lt},

		 {"le", std::vector<event_type_t>({event_type_t::boolean_event, event_type_t::boolean_event}),event_type_t::boolean_event, functions::le},
		 {"le", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::integer_event}),event_type_t::boolean_event, functions::le},
		 {"le", std::vector<event_type_t>({event_type_t::double_event, event_type_t::double_event}),event_type_t::boolean_event, functions::le},
		 {"le", std::vector<event_type_t>({event_type_t::string_event, event_type_t::string_event}),event_type_t::boolean_event, functions::le},
		 {"le", std::vector<event_type_t>({event_type_t::time_event, event_type_t::time_event}),event_type_t::boolean_event, functions::le},
		 {"le", std::vector<event_type_t>({event_type_t::duration_event, event_type_t::duration_event}),event_type_t::boolean_event, functions::le},

		 {"and", std::vector<event_type_t>({event_type_t::boolean_event, event_type_t::boolean_event}),event_type_t::boolean_event, functions::log_and},
		 {"or",  std::vector<event_type_t>({event_type_t::boolean_event, event_type_t::boolean_event}),event_type_t::boolean_event, functions::log_or},
		 {"not",  std::vector<event_type_t>({event_type_t::boolean_event}),event_type_t::boolean_event, functions::log_not},

		 {"and", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::integer_event}),event_type_t::integer_event, functions::bit_and},
		 {"or", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::integer_event}),event_type_t::integer_event, functions::bit_or},
		 {"xor", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::integer_event}),event_type_t::integer_event, functions::bit_xor},


		 {"min", std::vector<event_type_t>({event_type_t::boolean_event, event_type_t::boolean_event}),event_type_t::boolean_event, functions::min},
		 {"min", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::integer_event}),event_type_t::integer_event, functions::min},
		 {"min", std::vector<event_type_t>({event_type_t::double_event, event_type_t::double_event}),event_type_t::double_event, functions::min},
		 {"min", std::vector<event_type_t>({event_type_t::string_event, event_type_t::string_event}),event_type_t::string_event, functions::min},
		 {"min", std::vector<event_type_t>({event_type_t::time_event, event_type_t::time_event}),event_type_t::time_event, functions::min},
		 {"min", std::vector<event_type_t>({event_type_t::duration_event, event_type_t::duration_event}),event_type_t::duration_event, functions::min},

		 {"max", std::vector<event_type_t>({event_type_t::boolean_event, event_type_t::boolean_event}),event_type_t::boolean_event, functions::max},
		 {"max", std::vector<event_type_t>({event_type_t::integer_event, event_type_t::integer_event}),event_type_t::integer_event, functions::max},
		 {"max", std::vector<event_type_t>({event_type_t::double_event, event_type_t::double_event}),event_type_t::double_event, functions::max},
		 {"max", std::vector<event_type_t>({event_type_t::string_event, event_type_t::string_event}),event_type_t::string_event, functions::max},
		 {"max", std::vector<event_type_t>({event_type_t::time_event, event_type_t::time_event}),event_type_t::time_event, functions::max},
		 {"max", std::vector<event_type_t>({event_type_t::duration_event, event_type_t::duration_event}),event_type_t::duration_event, functions::max},

		 {"abs", std::vector<event_type_t>({event_type_t::integer_event}),event_type_t::integer_event, functions::abs},
		 {"abs", std::vector<event_type_t>({event_type_t::double_event}),event_type_t::double_event, functions::abs},
		 {"abs", std::vector<event_type_t>({event_type_t::duration_event}),event_type_t::duration_event, functions::abs},

		 {"exp", std::vector<event_type_t>({event_type_t::double_event}),event_type_t::double_event, functions::exp},
		 {"ln", std::vector<event_type_t>({event_type_t::double_event}),event_type_t::double_event, functions::ln},
		 {"pow", std::vector<event_type_t>({event_type_t::double_event, event_type_t::double_event}),event_type_t::double_event, functions::pow},
//		 {"min", std::vector<event_type_t>({event_type_t::vector_event}),event_type_t::undetermined_event, functions::min2},
//		 {"max", std::vector<event_type_t>({event_type_t::vector_event}),event_type_t::undetermined_event, functions::max2},


		 {"geometry", std::vector<event_type_t>({event_type_t::string_event}),event_type_t::string_event, functions::geometry},

		 {"geometry", std::vector<event_type_t>({event_type_t::undetermined_event, event_type_t::undetermined_event}),event_type_t::string_event, functions::geometry},
		 {"geometry", std::vector<event_type_t>({event_type_t::undetermined_event, event_type_t::undetermined_event, event_type_t::undetermined_event, event_type_t::undetermined_event}),event_type_t::string_event, functions::geometry},


		 {"get_width", std::vector<event_type_t>({event_type_t::integer_event}),event_type_t::integer_event, functions::get_width},
		 {"get_width", std::vector<event_type_t>({event_type_t::string_event}),event_type_t::integer_event, functions::get_width},
		 {"get_height", std::vector<event_type_t>({event_type_t::integer_event}),event_type_t::integer_event, functions::get_height},
		 {"get_height", std::vector<event_type_t>({event_type_t::string_event}),event_type_t::integer_event, functions::get_height},

		 {"microseconds", std::vector<event_type_t>({event_type_t::integer_event}),event_type_t::duration_event, functions::microseconds},
		 {"microseconds", std::vector<event_type_t>({event_type_t::double_event}),event_type_t::duration_event, functions::microseconds},
		 {"microseconds", std::vector<event_type_t>({event_type_t::duration_event}),event_type_t::duration_event, functions::microseconds},

		 {"milliseconds", std::vector<event_type_t>({event_type_t::integer_event}),event_type_t::duration_event, functions::milliseconds},
		 {"milliseconds", std::vector<event_type_t>({event_type_t::double_event}),event_type_t::duration_event, functions::milliseconds},
		 {"milliseconds", std::vector<event_type_t>({event_type_t::duration_event}),event_type_t::duration_event, functions::milliseconds},

		 {"seconds", std::vector<event_type_t>({event_type_t::integer_event}),event_type_t::duration_event, functions::seconds},
		 {"seconds", std::vector<event_type_t>({event_type_t::double_event}),event_type_t::duration_event, functions::seconds},
		 {"seconds", std::vector<event_type_t>({event_type_t::duration_event}),event_type_t::duration_event, functions::seconds},

		 {"generate", std::vector<event_type_t>({event_type_t::string_event}),event_type_t::string_event, functions::generate},


 };
}

}
}


