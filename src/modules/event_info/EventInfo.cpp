/*!
 * @file 		EventInfo.cpp
 * @author 		<Your name>
 * @date		11.07.2013
 * @copyright	Institute of Intermedia, 2013
 * 				Distributed under GNU Public License 3.0
 *
 */

#include "EventInfo.h"
#include "yuri/core/Module.h"

namespace yuri {
namespace event_info {

REGISTER("event_info",EventInfo)

IO_THREAD_GENERATOR(EventInfo)

core::pParameters EventInfo::configure()
{
	core::pParameters p = core::BasicIOThread::configure();
	p->set_description("EventInfo");
	p->set_max_pipes(1,1);
	return p;
}


EventInfo::EventInfo(log::Log &log_, core::pwThreadBase parent, core::Parameters &parameters):
core::BasicIOThread(log_,parent,0,0,std::string("event_info"))
{
	IO_THREAD_INIT("event_info")
	latency=1000;
}

EventInfo::~EventInfo()
{
}

void EventInfo::run()
{
	IO_THREAD_PRE_RUN
	while (still_running()) {
		process_events();
		sleep(latency);
	}
	IO_THREAD_POST_RUN
}
bool EventInfo::set_param(const core::Parameter& param)
{
	return core::BasicIOThread::set_param(param);
}

namespace {
	using namespace yuri::event;
	template<class Stream>
	Stream& print_event_info(const pBasicEvent& event, Stream& stream);

	template<class EventType, class Stream>
	Stream& event_info_detail(const shared_ptr<EventType> & /*event*/, Stream& stream)
	{
		return stream;
	}
	template<class Stream>
	Stream& event_info_detail(const shared_ptr<EventBang>& /*event*/, Stream& stream)
	{
		return stream << "BANG";
	}
	template<class Stream>
	Stream& event_info_detail(const shared_ptr<EventBool>& event, Stream& stream)
	{
		return stream << "BOOL: " << (event->get_value()?"True":"False");
	}

	template<class Stream, class EventType>
	Stream& event_info_ranged_detail(const shared_ptr<EventType>& event, const std::string& name, Stream& stream)
	{
		stream << name << ": " << event->get_value();
		if (event->range_specified()) {
			stream << " with range <"<<event->get_min_value() << ", " << event->get_max_value() << ">";
		}
		return stream;
	}
	template<class Stream>
	Stream& event_info_detail(const shared_ptr<EventInt>& event, Stream& stream)
	{
		return event_info_ranged_detail(event,"INTEGER: ",stream);
	}
	template<class Stream>
	Stream& event_info_detail(const shared_ptr<EventDouble>& event, Stream& stream)
	{
		return event_info_ranged_detail(event,"DOUBLE: ",stream);
	}
	template<class Stream>
	Stream& event_info_detail(const shared_ptr<EventString>& event, Stream& stream)
	{
		return stream << "STRING: " << event->get_value();
	}
	template<class Stream>
	Stream& event_info_detail(const shared_ptr<EventTime>& /*event*/, Stream& stream)
	{
		return stream << "TIME";//event->get_value() << "ns";
	}
	template<class Stream>
	Stream& event_info_detail(const shared_ptr<EventVector>& event, Stream& stream)
	{
		stream << "VECTOR: [ ";
		for (const auto& val: *event) {
			print_event_info(val, stream << "\n\t");
		}
		return stream << " ]";
	}
	template<class Stream>
	Stream& event_info_detail(const shared_ptr<EventDict>& event, Stream& stream)
	{
		stream << "MAP: { ";
		const auto& values = event->get_value();
		for (const auto& val: values) {
			print_event_info(val.second, stream << "\n\t" << val.first << ": ");
		}
		return stream << " }";
	}
	template<class EventType, class Stream>
	Stream& event_info_cast(const event::pBasicEvent& event, Stream& stream)
	{
		const auto& ev = dynamic_pointer_cast<EventType>(event);
		assert(ev);
		return event_info_detail(ev, stream);
	}

	template<class Stream>
	Stream& print_event_info(const event::pBasicEvent& event, Stream& stream)
	{

		switch (event->get_type()) {
			case event_type_t::bang_event: return event_info_cast<EventBang>(event, stream);
			case event_type_t::boolean_event: return event_info_cast<EventBool>(event, stream);
			case event_type_t::integer_event: return event_info_cast<EventInt>(event, stream);
			case event_type_t::double_event: return event_info_cast<EventDouble>(event, stream);
			case event_type_t::string_event: return event_info_cast<EventString>(event, stream);
			case event_type_t::time_event: return event_info_cast<EventTime>(event, stream);
			case event_type_t::vector_event: return event_info_cast<EventVector>(event, stream);
			case event_type_t::dictionary_event: return event_info_cast<EventDict>(event, stream);
			default: break;
		}
		return stream;
	}
}

bool EventInfo::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
	print_event_info(event, log[log::info] << "Received an event '" << event_name << "': ");
	return true;
}
} /* namespace event_info */
} /* namespace yuri */