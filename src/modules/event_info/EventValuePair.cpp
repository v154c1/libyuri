/*!
 * @file 		EventValuePair.cpp
 * @author 		Jiri Melnikov <melnikov@cesnet.cz>
 * @date 		11.06.2015
 * @copyright	CESNET z.s.p.o.
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "EventValuePair.h"
#include "yuri/core/Module.h"
#include <cassert>
namespace yuri {
namespace event_value_pair {

IOTHREAD_GENERATOR(EventValuePair)

core::Parameters EventValuePair::configure()
{
	core::Parameters p = core::IOThread::configure();
	p.set_description("If event name is \"line_nameline\", then it converts string \"name value\" into event named \"name\" with value \"value\" and otherwise.");
	p["line_name"]["Name of line event"]="line";
	p["separator"]["Separator of name and value. Following control sequences for special character are recognized: \\t, \\n, \\r, \\space, \\\\"]="\\space";
	return p;
}


EventValuePair::EventValuePair(log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::IOThread(log_,parent,0,0,std::string("event_value_pair")),
event::BasicEventConsumer(log),event::BasicEventProducer(log),
line_event_name_{"line"},value_separator_{" "}
{
	IOTHREAD_INIT(parameters);
	set_latency(50_ms);
	log[log::info] << "Using separator: >>" << value_separator_ << "<<";
}

EventValuePair::~EventValuePair() noexcept
{
}

void EventValuePair::run()
{
	while (still_running()) {
		wait_for_events(get_latency());
		process_events();
	}
}

namespace {
std::string replace_value(std::string val, const std::string& from, const std::string& to)
{
	std::string::size_type idx = 0;
	const auto fsize = from.size();
	const auto tsize = to.size();
	while ((idx = val.find(from, idx)) != val.npos) {
		val.replace(idx, fsize, to);
		idx = idx + tsize;
	}
	return val;
}

std::string fix_separator(std::string val)
{
	const std::unordered_map<std::string, std::string> control_characters = {
			{"\\n", "\n"},
			{"\\r", "\r"},
			{"\\t", "\t"},
			{"\\space", " "},
			{"\\\\", "\\"}
	};
	for (const auto&cc: control_characters) {
		val = replace_value(std::move(val), cc.first, cc.second);
	}
	return val;
}
}
bool EventValuePair::set_param(const core::Parameter& param)
{
	if (assign_parameters(param)
			(line_event_name_, "line_name"))
		return true;
	if (assign_parameters(param)
			(value_separator_, "separator")) {
		value_separator_ = fix_separator(value_separator_);
		return true;
	}
	return core::IOThread::set_param(param);
}


bool EventValuePair::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
	using namespace yuri::event;
	if (event_name == line_event_name_) {
		std::string message = event::get_value<event::EventString>(event);
		size_t pos = message.find(value_separator_);
		if (pos != std::string::npos) {
			std::string name = message.substr(0, pos);
			std::string value = message.substr(pos+1);
			auto pair_e = std::make_shared<EventString>(std::move(value));
			emit_event(name, std::move(pair_e));
		} else {
			log[log::warning] << "Value not found.";
		}
	} else {
		std::string message = event_name + value_separator_ + event::lex_cast_value<std::string>(event);
		auto line = std::make_shared<EventString>(std::move(message));
		emit_event(line_event_name_, std::move(line));
	}
	emit_event(event_name, event);
	return true;
}
} /* namespace event_value_pair */
} /* namespace yuri */
