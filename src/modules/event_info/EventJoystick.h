/*!
 * @file 		EventJoystick.h
 */

#ifndef EVENTJOYSTICK_H_
#define EVENTJOYSTICK_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/event/BasicEventProducer.h"
#include <linux/joystick.h>
namespace yuri {
namespace event_joystick {

class EventJoystick: public core::IOThread, public event::BasicEventProducer
{
public:
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	EventJoystick(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~EventJoystick() noexcept;
private:

	virtual void run() override;
	virtual bool set_param(const core::Parameter& param) override;
	std::string device_path_;
	int handle_;
};

} /* namespace event_joystick */
} /* namespace yuri */
#endif /* EVENTJOYSTICK_H_ */
