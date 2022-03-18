/*!
 * @file 		RepackAudio.cpp
 * @author 		Zdenek Travnicek
 * @date 		11.2.2013
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "RepackAudio.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_audio_frame_types.h"

namespace yuri {
namespace repack_audio {

//REGISTER("repack_audio",RepackAudio)

MODULE_REGISTRATION_BEGIN("repack_audio")
		REGISTER_IOTHREAD("repack_audio",RepackAudio)
MODULE_REGISTRATION_END()

IOTHREAD_GENERATOR(RepackAudio)

// So we can write log[info] instead of log[log::info]
//using namespace yuri::log;

core::Parameters RepackAudio::configure()
{
	core::Parameters p = base_type ::configure();
	p.set_description("Repack audio.");
	//(*p)["fps"]["Framerate of output"]=24.0;
	p["samples"]["Number of samples in output chunk"]=2000;
	p["channels"]["Number of channels in output chunk"]=2;
//	p->set_max_pipes(1,1);
	return p;
}


RepackAudio::RepackAudio(log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
base_type(log_,parent,std::string("repack")),samples_(2000,0)
,samples_missing_(2000),total_samples_(2000),channels_(2),current_format_(core::raw_audio_format::signed_16bit),
sampling_frequency_(48000)
{
	IOTHREAD_INIT(parameters)
}

RepackAudio::~RepackAudio() noexcept
{
}
size_t RepackAudio::store_samples(const uint8_t* start, size_t count, size_t sample_size)
{
	if (!total_samples_) return count;
	size_t stored = 0;
	while (count > 0) {
		const size_t to_copy = std::min(count, samples_missing_);
		const size_t sample_offset = total_samples_ - samples_missing_;
		std::copy(start,start+sample_size*to_copy*channels_,samples_.begin()+sample_size*sample_offset*channels_);
		count -= to_copy;
		stored += to_copy;
		start+=to_copy*channels_*sample_size;
		samples_missing_ -= to_copy;
		if (!samples_missing_) push_current_frame(sample_size);
	}
	return stored;

}

void RepackAudio::push_current_frame(size_t sample_size) {
	auto f = core::RawAudioFrame::create_empty(current_format_, channels_, sampling_frequency_, &samples_[0], total_samples_*sample_size*channels_);
	push_frame(0,f);
	samples_missing_ = total_samples_;
}

core::pFrame RepackAudio::do_special_single_step(core::pRawAudioFrame frame) {
	current_format_ = frame->get_format();
	if (current_format_!= core::raw_audio_format::signed_16bit &&
			current_format_ != core::raw_audio_format::unsigned_16bit &&
			current_format_ != core::raw_audio_format::signed_32bit   &&
			current_format_ != core::raw_audio_format::unsigned_32bit) {
		log[log::warning] << "Unsupported format. Only 16bit/32bit formats supported";
		return {};
	}
	if (frame->get_channel_count() != channels_) {
		log[log::warning] << "Expected " << channels_<<" channels, but got: " << frame->get_channel_count();
		return {};
	}
	sampling_frequency_ = frame->get_sampling_frequency();
	const uint8_t* data = frame->data();
	size_t available_samples = frame->get_sample_count();
	size_t sample_size = frame->get_sample_size()/frame->get_channel_count()/8;
	samples_.resize(total_samples_*channels_*sample_size,0);
	store_samples(data, available_samples, sample_size);
	return {};
}
bool RepackAudio::set_param(const core::Parameter& param)
{
	if (param.get_name() == "samples") {
		samples_missing_ = param.get<size_t>();
		total_samples_ = samples_missing_;
	} else if (param.get_name() == "channels") {
		channels_ = param.get<size_t>();
	} else return base_type::set_param(param);
	return true;
}

} /* namespace dummy_module */
} /* namespace yuri */
