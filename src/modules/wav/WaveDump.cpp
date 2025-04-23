/*!
 * @file 		WaveDump.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		20.03.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "WaveDump.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/raw_audio_frame_types.h"
#include "yuri/core/utils/DirectoryBrowser.h"
#include "yuri/core/utils/string_generator.h"
namespace yuri {
namespace wav {


IOTHREAD_GENERATOR(WaveDump)

MODULE_REGISTRATION_BEGIN("wav")
		REGISTER_IOTHREAD("wav_dump",WaveDump)
MODULE_REGISTRATION_END()

core::Parameters WaveDump::configure()
{
	core::Parameters p = base_type::configure();
	p.set_description("WaveDump");
	p["filename"]["Output filename"]="audio.wav";
	return p;
}


WaveDump::WaveDump(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
base_type(log_,parent,std::string("wav")),event::BasicEventProducer(log),
filename_("xxx.wav"),format_set_(false),seq_number_(0)
{
	IOTHREAD_INIT(parameters)

	auto filename = core::utils::generate_string(filename_);
	core::filesystem::ensure_path_directory(filename);
	file_.open(filename, std::ios::out| std::ios::binary);
	if (!file_.is_open()) {
		throw exception::InitializationFailed("Failed to open output file");
	}

}

WaveDump::~WaveDump() noexcept
{
}

core::pFrame WaveDump::do_special_single_step(core::pRawAudioFrame frame)
{
	const auto fmt = frame->get_format();
	if (
		fmt != core::raw_audio_format::signed_16bit && 
		fmt != core::raw_audio_format::signed_24bit) {
		log[log::warning] << "Unsupported format!";
		return {};
	}
	const auto chan  = frame->get_channel_count();
	const auto sampl = frame->get_sampling_frequency();
	const auto bps   = frame->get_sample_size();
	if (!format_set_) {
		header_ = wav_header_t(chan, sampl, bps/2);
		format_set_ = true;
	} else {
		// Check for format change...
	}

	const auto new_size = frame->size();
	header_.add_size(static_cast<uint32_t>(new_size));
	file_.seekp(0,std::ios::beg);
	file_.write(reinterpret_cast<char*>(&header_),sizeof(header_));
	file_.seekp(0,std::ios::end);
	file_.write(reinterpret_cast<char*>(frame->data()),new_size);
	if (!info_string_.empty()) {
		emit_event("info", core::utils::generate_string(info_string_, seq_number_, frame));
	}
	seq_number_++;


	return {};
}

bool WaveDump::set_param(const core::Parameter& param)
{
	if (assign_parameters(param)
			(filename_, "filename")
			(info_string_, 	"info_string")) {
		return true;
	}
	return base_type::set_param(param);
}

} /* namespace wav */
} /* namespace yuri */
