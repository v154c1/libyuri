/*
 * FpsFixer.cpp
 *
 *  Created on: Aug 16, 2010
 *      Author: neneko
 */

#include "FpsFixer.h"
#include "yuri/core/Module.h"

namespace yuri {

namespace fps {


MODULE_REGISTRATION_BEGIN("fix_fps")
	REGISTER_IOTHREAD("fix_fps",FpsFixer)
MODULE_REGISTRATION_END()

IOTHREAD_GENERATOR(FpsFixer)

core::Parameters FpsFixer::configure()
{
	core::Parameters p = core::IOThread::configure();
	p["fps"]["FPS"]=25.0;
	return p;
}


FpsFixer::FpsFixer(log::Log &log_, core::pwThreadBase parent, const core::Parameters& parameters):
		IOThread(log_,parent,1,1,"FpsFixer"),event::BasicEventConsumer(log),
		fps_(25.0)
{
	IOTHREAD_INIT(parameters)
	set_latency(100_ms/fps_);
}

FpsFixer::~FpsFixer() noexcept
{
}

void FpsFixer::run()
{
	IOThread::print_id();
	core::pFrame frame;

	frames_ = 0;
	timer_.reset();
	while(still_running()) {
		process_events();
		while (auto f = pop_frame(0)) {
			frame = f;
		}
		if (timer_.get_duration() > frames_ * 1_s/fps_) {
			if (frame) {
				push_frame(0,frame);
			}
			frames_++;
		} else {
			sleep(get_latency());
		}
	}

}

bool FpsFixer::set_param(const core::Parameter &parameter)
{
	if (parameter.get_name() == "fps") {
		fps_=parameter.get<double>();
	} else return IOThread::set_param(parameter);
	return true;
}

bool FpsFixer::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
	try {
		if (event_name == "fps") {
			fps_ = event::lex_cast_value<float>(event);
			frames_ = 0;
			timer_.reset();
		} else return false;
	}
	catch (std::bad_cast&) {
		log[log::info] << "bad cast in " << event_name;
		return false;
	}
	return true;
}
}

}
