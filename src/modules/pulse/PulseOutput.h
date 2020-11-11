/*!
 * @file 		PulseOutput.h
 * @author 		Jiri Melnikov <jiri@melnikoff.org>
 * @date 		11.6.2019
 * @copyright	
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef PULSEOUTPUT_H_
#define PULSEOUTPUT_H_

#include "yuri/core/thread/SpecializedIOFilter.h"
#include "yuri/core/thread/InputThread.h"
#include "yuri/core/frame/RawAudioFrame.h"
#include <pulse/pulseaudio.h>

namespace yuri {
namespace pulse {

class PulseOutput: public core::SpecializedIOFilter<core::RawAudioFrame>
{
public:
	IOTHREAD_GENERATOR_DECLARATION
	PulseOutput(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~PulseOutput() noexcept;
	static core::Parameters configure();
	static std::vector<core::InputDeviceInfo> enumerate();
private:
	
	virtual core::pFrame do_special_single_step(core::pRawAudioFrame frame) override;
	virtual bool set_param(const core::Parameter& param) override;
	
	bool is_different_format(const core::pRawAudioFrame& frame);
	bool set_format(const core::pRawAudioFrame& frame);
	bool init_pulse();
	void destroy_pulse();

	std::string device_name_;
	format_t format_;
	size_t channels_;
	size_t forced_channels_;
	unsigned int sample_rate_;

	std::vector<uint8_t> channel_buffer_;


	pa_context *ctx_;
	pa_threaded_mainloop *pulse_loop_;
	pa_stream *stream_;
	int pulse_ready_;

};

} /* namespace pulse */
} /* namespace yuri */
#endif /* PULSEOUTPUT_H_ */
