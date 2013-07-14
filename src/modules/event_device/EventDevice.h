/*!
 * @file 		EventDevice.h
 * @author 		<Your name>
 * @date 		11.07.2013
 * @copyright	Institute of Intermedia, 2013
 * 				Distributed under GNU Public License 3.0
 *
 */

#ifndef EVENTDEVICE_H_
#define EVENTDEVICE_H_

#include "yuri/core/BasicIOThread.h"
#include "yuri/event/BasicEventProducer.h"
#include <linux/input.h>
namespace yuri {
namespace event_device {

struct abs_axis {
        int code;
        input_absinfo info;
        int last_value;
        int fuzz;
        std::string name;
};

class EventDevice: public core::BasicIOThread, public event::BasicEventProducer
{
public:
	IO_THREAD_GENERATOR_DECLARATION
	static core::pParameters configure();
	virtual ~EventDevice();
private:
	EventDevice(log::Log &log_, core::pwThreadBase parent, core::Parameters &parameters);
	virtual void run();
	virtual bool set_param(const core::Parameter& param);
	std::string device_path_;
	int							handle_;
	int							min_fuzz_;
	std::map<int, abs_axis> 	axis_info_;
};

} /* namespace event_device */
} /* namespace yuri */
#endif /* EVENTDEVICE_H_ */