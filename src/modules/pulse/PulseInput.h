/*!
 * @file 		PulseInput.h
 * @author 		Jiri Melnikov <jiri@melnikoff.org>
 * @date 		24.9.2020
 * @copyright
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef PULSEINPUT_H_
#define PULSEINPUT_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/core/thread/InputThread.h"
#include "yuri/core/frame/RawAudioFrame.h"
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>

namespace yuri {
namespace pulse {

class PulseInput: public core::IOThread {
public:
	IOTHREAD_GENERATOR_DECLARATION
	PulseInput(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~PulseInput() noexcept;
    static core::Parameters configure();
    static std::vector<core::InputDeviceInfo> enumerate();
private:

	virtual void run() override;
	virtual bool set_param(const core::Parameter& param) override;

	bool set_format();
	void destroy_pulse();

	std::string device_name_;
	format_t format_;
	size_t channels_;
	size_t forced_channels_;
	unsigned int sample_rate_;
	unsigned int frames_;
	unsigned int latency_;

	std::vector<uint8_t> data_;

	pa_simple *pa_s_;
};

} /* namespace pulse */
} /* namespace yuri */
#endif /* PULSEINPUT_H_ */
