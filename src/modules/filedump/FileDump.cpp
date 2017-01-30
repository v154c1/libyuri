/*!
 * @file 		FileDump.cpp
 * @author 		Zdenek Travnicek
 * @date 		24.7.2010
 * @date		04.2.2014
 * @copyright	Institute of Intermedia, CTU in Prague, 2010 - 2014
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "FileDump.h"
#include <sstream>
#include "yuri/core/Module.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/CompressedVideoFrame.h"
#include "yuri/core/frame/RawAudioFrame.h"
#include "yuri/core/frame/EventFrame.h"
#include "yuri/core/utils/string_generator.h"
#include "yuri/core/utils/DirectoryBrowser.h"
#include "yuri/core/utils/assign_events.h"
namespace yuri
{
namespace dump
{

MODULE_REGISTRATION_BEGIN("filedump")
	REGISTER_IOTHREAD("filedump",FileDump)
MODULE_REGISTRATION_END()

IOTHREAD_GENERATOR(FileDump)

core::Parameters FileDump::configure()
{
	core::Parameters p = IOFilter::configure();
	p.set_description("Outputs frames to a file. It either dump all frames to one file of every frame to separate file. Filename can contain a format specifiers in a form %n, %t, %T or %05s");
	p["sequence"]["Number of digits in sequence number. Set to 0 to disable sequence and output all frames to one file."]=0;
	p["filename"]["Required parameter. Path of file to dump to"]=std::string();
	p["frame_limit"]["Maximal number of frames to dump. 0 for unlimited"]=0;
	p["info_string"]["Additional string to emit with each frame (as event 'info')"]="";
	p["append"]["Append to the end of file"]=false;
	return p;
}

namespace {

template<class T>
std::string append_to_filename(const std::string& filename, const T& value, int width = 0)
{
	std::stringstream ss;
	const auto dot_index = filename.find_last_of('.');
	if (dot_index != std::string::npos) {
		ss << filename.substr(0,dot_index);
	} else {
		ss << filename;
	}
	core::utils::print_formated_value(ss, value, width);
	if (dot_index != std::string::npos) {
		ss << filename.substr(dot_index);
	}
	return ss.str();
}


}



FileDump::FileDump(log::Log &log_,core::pwThreadBase parent, const core::Parameters &parameters):
	IOFilter(log_,parent,"Dump"),
	event::BasicEventProducer(log),
	event::BasicEventConsumer(log),
	dump_file(),filename(),seq_chars(0),seq_number(0),dumped_frames(0),
	dump_limit(0),use_regex_(false),single_file_(true),append_(false)
{
	IOTHREAD_INIT(parameters);
	if (filename.empty()) throw exception::InitializationFailed("No filename specified");

	if (core::utils::is_extended_generator_supported()) {
		auto s = core::utils::analyze_string_specifiers(filename);
		use_regex_ = s.first;
		single_file_ = !s.second;
		if (single_file_ && seq_chars) {
			std::string spec = "%0"+std::to_string(seq_chars)+"s";
			filename = append_to_filename(filename, spec);
			log[log::info] << "Expanded filename to " << filename;
			use_regex_ = true;
			single_file_ = false;
		}
		// The filename has to be generated beforehand, when single_file is enabled.
		if (use_regex_ && single_file_) {
			filename = generate_filename({});
			use_regex_ = false;
		}
	} else {
		single_file_ = seq_number <= 0;
	}

	if (single_file_) {
		open_file(filename);
	}
}

FileDump::~FileDump() noexcept
{
	if (dump_file.is_open()) dump_file.close();
}

bool FileDump::open_file(const std::string& fname)
{
	core::filesystem::ensure_path_directory(fname);
	auto flags = std::ios::binary | std::ios::out;
	if (append_) flags|=std::ios::app;
	if (dump_file.is_open()) dump_file.close();
	dump_file.open(fname.c_str(), flags);
	return true;
}
std::string FileDump::generate_filename(const core::pFrame& frame)
{
	if (!use_regex_ && !single_file_) {
		return append_to_filename(filename, seq_number++, seq_chars);
	}
	else {
		return core::utils::generate_string(filename, seq_number, frame);
	}
	return filename;
}

core::pFrame FileDump::do_simple_single_step(core::pFrame frame)
{
	process_events();
	if (dumped_frames >= dump_limit && dump_limit) {
		return {};
	}
	if (!single_file_) {
		const auto seq_filename = generate_filename(frame);
		log[log::debug] << "New filename " << seq_filename;
		open_file(seq_filename);
		emit_event("filename",seq_filename);
	}
	bool written = true;
	if (auto f = std::dynamic_pointer_cast<core::RawVideoFrame>(frame)) {
		log[log::debug]<<"Dumping " << f->get_planes_count() << " planes";
		for (yuri::size_t i=0; i<f->get_planes_count();++i) {
			dump_file.write(reinterpret_cast<const char *>(PLANE_RAW_DATA(f,0)),PLANE_SIZE(f,0));
		}
	} else if (auto f2 = std::dynamic_pointer_cast<core::CompressedVideoFrame>(frame)) {
		dump_file.write(reinterpret_cast<const char *>(f2->begin()),f2->size());
	} else if (auto f3 = std::dynamic_pointer_cast<core::RawAudioFrame>(frame)) {
		dump_file.write(reinterpret_cast<const char *>(f3->data()),f3->size());
	} else if (auto f4 = std::dynamic_pointer_cast<core::EventFrame>(frame)) {
		try {
			const auto text = event::lex_cast_value<std::string>(f4->get_event()) +"\n";
			dump_file.write(text.c_str(),text.size());
		}
		catch (std::exception& e) {
			log[log::warning] << "Failed to store event " << f4->get_name();
			written = false;
		}
	} else {
		written=false;
	}
	if (!info_string_.empty()) {
		emit_event("info", core::utils::generate_string(info_string_, seq_number, frame));
	}
	if (!single_file_) {
		dump_file.close();
	}
	if (written) {
		emit_event("frame");
		emit_event("sequence",seq_number);
	}
	++seq_number;

	// The comparison is evaluated FIRST in order to have dumped_frames counted even if dump_limit is zero
	if (++dumped_frames >= dump_limit && dump_limit) {
		log[log::debug] << "Maximal number of frames reached, quitting.";
		request_end();
	}

	return {};
}

bool FileDump::set_param(const core::Parameter &param)
{
	if (assign_parameters(param)
			(filename, 		"filename")
			(seq_chars, 	"sequence")
			(dump_limit, 	"frame_limit")
			(info_string_, 	"info_string")
			(append_,		"append"))
		return true;
	return IOFilter::set_param(param);
}

bool FileDump::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
	if (assign_events(event_name, event)
		(filename, "filename")) {
		auto s = core::utils::analyze_string_specifiers(filename);
		use_regex_ = s.first;
		single_file_ = !s.second;

		if (single_file_) {
			// The filename has to be generated beforehand, when single_file is enabled.
			if (use_regex_) {
				filename = generate_filename({});
				use_regex_ = false;
			}
			open_file(filename);
			seq_number = 0;
		}
		return true;
	}
	return false;
}

}
}
//End of File

