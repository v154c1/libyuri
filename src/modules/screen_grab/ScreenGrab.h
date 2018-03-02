/*
 * ScreenGrab.h
 *
 *  Created on: 7.3.2013
 *      Author: neneko
 */

#ifndef SCREENGRAB_H_
#define SCREENGRAB_H_

#include "yuri/core/thread/IOThread.h"
#include "X11/Xlib.h"
namespace yuri {
namespace screen {

class ScreenGrab: public core::IOThread
{
public:
	virtual ~ScreenGrab() noexcept;
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	ScreenGrab(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
private:

	virtual void run() override;
	virtual bool step() override;
	virtual bool set_param(const core::Parameter &param) override;
	std::string display;
	double fps_;
	std::shared_ptr<Display> dpy;
	Window win;
	coordinates_t position_;
	resolution_t resolution_;
	bool cursor_;
	std::string win_name;
	size_t pid;
	Window win_id_;

};

} /* namespace screen */
} /* namespace yuri */


#endif /* SCREENGRAB_H_ */
