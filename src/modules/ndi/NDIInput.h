/*
 * NDIInput.h
  */

#ifndef NDIINPUT_H_
#define NDIINPUT_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/event/BasicEventConsumer.h"
#include "yuri/core/thread/InputThread.h"

#include <Processing.NDI.Lib.h>

namespace yuri {

namespace ndi {

class NDIInput:public core::IOThread {
public:
	NDIInput(log::Log &log_,core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~NDIInput() noexcept;
	virtual void run() override;
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	//static std::vector<core::InputDeviceInfo> enumerate();
private:
	virtual bool set_param(const core::Parameter &param) override;

	std::string stream_;
};

}

}

#endif /* NDIINPUT_H_ */
