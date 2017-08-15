/*!
 * @file 		AlsaOutput.h
 * @author 		Jiri Melnikov <jiri@melnikoff.org>
 * @date 		01.10.2017
 * @copyright
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef ALSAINPUT_H_
#define ALSAINPUT_H_

#include "yuri/core/thread/IOThread.h"
#include "yuri/core/frame/RawAudioFrame.h"
#include <alsa/asoundlib.h>

namespace yuri {
namespace alsa {

class AlsaInput: public core::IOThread
{
public:
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	AlsaInput(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~AlsaInput() noexcept;
private:
	
	virtual void run() override;
	virtual bool set_param(const core::Parameter& param) override;

	bool init_alsa();

	bool error_call(int, const std::string&);
	format_t format_;
	std::string device_name_;
	size_t channels_;
	unsigned int sample_rate_;
	unsigned int frames_;
	unsigned int frames_max_size_;

	snd_pcm_t *handle_;

	std::vector<uint8_t> data_;
};

} /* namespace alsa */
} /* namespace yuri */
#endif /* ALSAINPUT_H_ */
