/*!
 * @file 		RepackAudio.h
 * @author 		Zdenek Travnicek
 * @date		17.5.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef DUMMYMODULE_H_
#define DUMMYMODULE_H_

#include "yuri/core/thread/SpecializedIOFilter.h"
#include "yuri/core/frame/RawAudioFrame.h"
namespace yuri {
namespace repack_audio {

class RepackAudio: public core::SpecializedIOFilter<core::RawAudioFrame>
{
	using base_type = core::SpecializedIOFilter<core::RawAudioFrame>;
public:
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	RepackAudio(log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~RepackAudio() noexcept;
private:

//	virtual bool step();
	virtual core::pFrame do_special_single_step(core::pRawAudioFrame frame) override;
	virtual bool set_param(const core::Parameter& param) override;
	size_t store_samples(const uint8_t* start, size_t count, size_t sample_size);
	void push_current_frame(size_t sample_size);
	std::string dummy_name;
	std::vector<uint8_t> samples_;
	size_t samples_missing_;
	size_t total_samples_;
	size_t channels_;
	format_t current_format_;
	size_t sampling_frequency_;
};

} /* namespace repack_audio */
} /* namespace yuri */
#endif /* DUMMYMODULE_H_ */
