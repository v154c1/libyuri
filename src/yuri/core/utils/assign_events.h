/*!
 * @file 		assign_events.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		29.01.2015
 * @copyright	Institute of Intermedia, 2015
 * 				Distributed BSD License
 *
 */


#ifndef ASSIGN_EVENTS_H_
#define ASSIGN_EVENTS_H_
#include "yuri/event/EventHelpers.h"

namespace yuri {
struct assign_events {
	assign_events(const std::string& event_name, const event::pBasicEvent& event):
		event_name(event_name),event(event), assigned(false) {}

	template<class T>
	assign_events& operator()(T&) {
		return *this;
	}
	template<class T, class Str, class... Args>
	assign_events& operator()(T& target, Str&& name, Args&&... names)
	{
		if (event_name == name) {
			target = event::lex_cast_value<T>(event);
			assigned = true;
		} else {
			return (*this)(target, names...);
		}
		return *this;
	}


	template<class T, class T2>
	typename std::enable_if<std::is_arithmetic<T>::value && std::is_arithmetic<T2>::value, assign_events&>::type
	ranged(T&, const T2&, const T2&)
	{
		return *this;
	}

	template<class T, class T2, class Str, class... Args>
	typename std::enable_if<std::is_arithmetic<T>::value && std::is_arithmetic<T2>::value, assign_events&>::type
	ranged(T& target, const T2& min, const T2& max, Str&& name, Args&&... names)
	{
		if (event_name == name) {
			if (auto float_event = std::dynamic_pointer_cast<event::EventDouble>(event)) {
				if (float_event->range_specified()) {
					const auto& range_min = float_event->get_min_value();
					const auto& range_max = float_event->get_max_value();
					const auto& range_size = range_max - range_min;
					const auto& new_range = max - min;
					target = static_cast<T>(min + new_range*(float_event->get_value() - range_min)/range_size);
				} else {
					target = std::min(std::max(event::lex_cast_value<T>(event), static_cast<T>(min)),static_cast<T>(max));
				}
				assigned = true;
				return *this;
			} else if (auto int_event = std::dynamic_pointer_cast<event::EventInt>(event)) {
				if (float_event->range_specified()) {
					const auto& range_min = int_event->get_min_value();
					const auto& range_max = int_event->get_max_value();
					const auto& range_size = range_max - range_min;
					const auto& new_range = max - min;
					target = static_cast<T>(min + new_range*(int_event->get_value() - range_min)/range_size);
				} else {
					target = std::min(std::max(event::lex_cast_value<T>(event), static_cast<T>(min)),static_cast<T>(max));
				}
				assigned = true;
				return *this;
			}
		}
		return (*this)(target, names...);
	}
	/*!
	 * Converts incoming vector into several values
	 *
	 * The syntax has to be reversed compared to other methods because of the variadic template...
	 * @param name expected argument name
	 * @param args targets for the values
	 * @return
	 */

	template<class Str, class... Arg>
	assign_events& vector_values(Str&& name, Arg&... args)
	{
		if (name != event_name) return *this;
		if (const auto& vec_event = std::dynamic_pointer_cast<event::EventVector>(event))
		{
			const auto& vec = vec_event->get_value();
			if (vec.size() < sizeof...(args)) return *this;
			return vector_values_impl(vec.cbegin(), vec.end(), args...);
		}
		return *this;
	}
	template<class Str, class Func>
	assign_events& bang(Str&& name, Func func) {
		if (name == event_name) {
			func();
			assigned = true;
		}
		return *this;
	}
	operator bool() { return assigned; }
	template<class P, class T, class Str, class F>
	assign_events&
	parsed(T& target, Str&& name, F f)
	{
		if (!assigned && name == event_name) {
			target = f(event::lex_cast_value<P>(event));
			assigned = true;
		}
		return *this;
	}
private:
	template<class It, class It2>
	assign_events& vector_values_impl(It, It2)
	{
		return *this;
	}
	template<class It, class It2, class First, class... Arg>
	assign_events& vector_values_impl(It iter, It2 last, First& first, Arg&... rest)
	{
        if (iter == last) {
            return *this;
        }
		first = event::lex_cast_value<First>(*iter++);
		return vector_values_impl(iter, last, rest...);
	}
private:
	const std::string& event_name;
	const event::pBasicEvent& event;
	bool assigned;
};

}



#endif /* ASSIGN_EVENTS_H_ */
