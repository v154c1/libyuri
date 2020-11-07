/*!
 * @file 		EventJoystick.cpp
 */

#include "EventJoystick.h"
#include "yuri/core/Module.h"
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
namespace yuri {
namespace event_joystick {

const size_t max_axis = 3;

IOTHREAD_GENERATOR(EventJoystick)

core::Parameters EventJoystick::configure()
{
	auto p = core::IOThread::configure();
	p.set_description("EventJoystick");
	p["device"]["Path to the joystick device"]="/dev/input/js0";
	return p;
}


namespace {

struct axis_state {
    int x, y, min_x, max_x, min_y, max_y;
};

size_t get_axis_state(struct js_event *event, struct axis_state axes[max_axis]) {
    size_t axis = event->number / 2;
    if (axis < max_axis) {
        if (event->number % 2 == 0) {
            axes[axis].x = event->value;
			if (axes[axis].x > axes[axis].max_x) axes[axis].max_x = axes[axis].x;
			else if (axes[axis].x < axes[axis].min_x) axes[axis].min_x = axes[axis].x;
        } else {
            axes[axis].y = event->value;
			if (axes[axis].y > axes[axis].max_y) axes[axis].max_y = axes[axis].y;
			else if (axes[axis].y < axes[axis].min_y) axes[axis].min_y = axes[axis].y;
		}
    }
    return axis;
}

}

EventJoystick::EventJoystick(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::IOThread(log_,parent,1,1,std::string("event_joystick")), event::BasicEventProducer(log), handle_(-1) {
	IOTHREAD_INIT(parameters)
	if ((handle_ = ::open(device_path_.c_str(), O_RDONLY)) < 0 ) {
		throw exception::InitializationFailed("Failed to open device "+device_path_);
	}
}

EventJoystick::~EventJoystick() noexcept {
}

void EventJoystick::run() {
	using namespace yuri::event;
	struct axis_state axis[max_axis] = {0,0,0,0,0,0};
	size_t curr_axis;

	while (still_running()) {
		pollfd fd = {handle_, POLLIN, 0};
		::poll(&fd, 1, get_latency().value/1000);
		if (fd.revents & POLLIN) {
			js_event ev;
			if (::read(handle_,&ev,sizeof(js_event))>0) {
				pBasicEvent event;
				switch (ev.type) {
					case JS_EVENT_AXIS:
						curr_axis = get_axis_state(&ev, axis);
						log[log::debug] << "Axis " << static_cast<size_t>(curr_axis) << " on " << axis[curr_axis].x << "x" << axis[curr_axis].y;
						event = std::make_shared<EventInt>(axis[curr_axis].x, axis[curr_axis].min_x, axis[curr_axis].max_x);
						emit_event("axis_"+std::to_string(curr_axis)+"_x", event);
						event = std::make_shared<EventInt>(axis[curr_axis].y, axis[curr_axis].min_y, axis[curr_axis].max_y);
						emit_event("axis_"+std::to_string(curr_axis)+"_y", event);
						break;
					case JS_EVENT_BUTTON:
						log[log::debug] << "Button " << static_cast<size_t>(ev.number) << " " << (ev.value ? "pressed" : "released");
						event = std::make_shared<EventBool>(ev.value);
						emit_event("btn_"+std::to_string(ev.number), event);
						break;
				}
			}
		}

	}
}

bool EventJoystick::set_param(const core::Parameter& param) {
	if (assign_parameters(param)
			(device_path_, "device"))
		return true;
	return core::IOThread::set_param(param);
}

} /* namespace event_joystick */
} /* namespace yuri */
