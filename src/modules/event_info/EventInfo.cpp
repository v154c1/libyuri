/*!
 * @file 		EventInfo.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		11.07.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "EventInfo.h"
#include "yuri/core/Module.h"
#include <cassert>
namespace yuri {
namespace event_info {

IOTHREAD_GENERATOR(EventInfo)


core::Parameters EventInfo::configure()
{
	core::Parameters p = core::IOThread::configure();
	p.set_description("EventInfo");
	p["enabled"]["Enable/disable the output."]=true;
	return p;
}


EventInfo::EventInfo(log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::IOThread(log_,parent,0,0,std::string("event_info")),
event::BasicEventConsumer(log),enabled_(true)
{
	IOTHREAD_INIT(parameters);
	set_latency(100_ms);
}

EventInfo::~EventInfo() noexcept
{
}

void EventInfo::run()
{
	while (still_running()) {
		wait_for_events(get_latency());
		process_events();
	}
}
bool EventInfo::set_param(const core::Parameter& param)
{
	if (param.get_name() == "enabled") {
		enabled_ = param.get<bool>();
	} else return core::IOThread::set_param(param);
	return true;
}

namespace {
	using namespace yuri::event;
	template<class Stream>
	void print_event_info(const pBasicEvent& event, Stream& stream);

	template<class EventType, class Stream>
	void event_info_detail(const std::shared_ptr<EventType> & /*event*/, Stream& /*stream*/)
	{
//		return stream;
	}
	template<class Stream>
	void event_info_detail(const std::shared_ptr<EventBang>& /*event*/, Stream& stream)
	{
		stream << "BANG";
	}
	template<class Stream>
	void event_info_detail(const std::shared_ptr<EventBool>& event, Stream& stream)
	{
		stream << "BOOL: " << (event->get_value()?"True":"False");
	}

	template<class Stream, class EventType>
	void event_info_ranged_detail(const std::shared_ptr<EventType>& event, const std::string& name, Stream& stream)
	{
		stream << name << ": " << event->get_value();
		if (event->range_specified()) {
			stream << " with range <"<<event->get_min_value() << ", " << event->get_max_value() << ">";
		}
//		return stream;
	}
	template<class Stream>
	void event_info_detail(const std::shared_ptr<EventInt>& event, Stream& stream)
	{
		event_info_ranged_detail(event,"INTEGER: ",stream);
	}
	template<class Stream>
	void event_info_detail(const std::shared_ptr<EventDouble>& event, Stream& stream)
	{
		event_info_ranged_detail(event,"DOUBLE: ",stream);
	}
	template<class Stream>
	void event_info_detail(const std::shared_ptr<EventString>& event, Stream& stream)
	{
		stream << "STRING: " << event->get_value();
	}
	template<class Stream>
	void event_info_detail(const std::shared_ptr<EventDuration>& event, Stream& stream)
	{
		stream << "DURATION: " << event->get_value();
	}
	template<class Stream>
	void event_info_detail(const std::shared_ptr<EventTime>& /*event*/, Stream& stream)
	{
		stream << "TIME";//event->get_value() << "ns";
	}
	template<class Stream>
	void event_info_detail(const std::shared_ptr<EventVector>& event, Stream& stream)
	{
		stream << "VECTOR: [ ";
		for (const auto& val: *event) {
			print_event_info(val, stream << "\n\t");
		}
		stream << " ]";
	}
	template<class Stream>
	void event_info_detail(const std::shared_ptr<EventDict>& event, Stream& stream)
	{
		stream << "MAP: { ";
		const auto& values = event->get_value();
		for (const auto& val: values) {
			print_event_info(val.second, stream << "\n\t" << val.first << ": ");
		}
		stream << " }";
	}
	template<class EventType, class Stream>
	void event_info_cast(const event::pBasicEvent& event, Stream& stream)
	{
		const auto& ev = std::dynamic_pointer_cast<EventType>(event);
		assert(ev);
		event_info_detail(ev, stream);
	}

	template<class Stream>
	void print_event_info(const event::pBasicEvent& event, Stream& stream)
	{

		switch (event->get_type()) {
			case event_type_t::bang_event: return event_info_cast<EventBang>(event, stream);
			case event_type_t::boolean_event: return event_info_cast<EventBool>(event, stream);
			case event_type_t::integer_event: return event_info_cast<EventInt>(event, stream);
			case event_type_t::double_event: return event_info_cast<EventDouble>(event, stream);
			case event_type_t::string_event: return event_info_cast<EventString>(event, stream);
			case event_type_t::time_event: return event_info_cast<EventTime>(event, stream);
			case event_type_t::duration_event: return event_info_cast<EventDuration>(event, stream);
			case event_type_t::vector_event: return event_info_cast<EventVector>(event, stream);
			case event_type_t::dictionary_event: return event_info_cast<EventDict>(event, stream);
			default: break;
		}
//		stream;
	}
}

bool EventInfo::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
	if (enabled_) {
		// This move is necessary only for GCC4.7 which doesn't support ref-qualified member functions
		auto l = log[log::info];
        l << "Received an event '" << event_name << "': ";
		print_event_info(event, l);
	}
	return true;
}
} /* namespace event_info */
} /* namespace yuri */
