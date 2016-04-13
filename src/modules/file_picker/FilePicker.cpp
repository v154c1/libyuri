/*!
 * @file 		FilePicker.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		21.02.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "FilePicker.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/CompressedVideoFrame.h"
//#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/raw_frame_params.h"
//#include "yuri/core/frame/compressed_frame_types.h"
#include "yuri/core/frame/compressed_frame_params.h"
#include "yuri/core/utils/assign_events.h"

#include <boost/regex.hpp>
namespace yuri {
namespace file_picker {


IOTHREAD_GENERATOR(FilePicker)

MODULE_REGISTRATION_BEGIN("file_picker")
		REGISTER_IOTHREAD("file_picker",FilePicker)
MODULE_REGISTRATION_END()

core::Parameters FilePicker::configure()
{
	core::Parameters p = core::IOThread::configure();
	p.set_description("FilePicker");
	p["pattern"]["Pattern for files to load. It should contain %xd or %0xd (where x is some number) to specify the sequence."]="";
	p["index"]["Initial index"]=0;
	p["fps"]["Output framerate. Set to zero to output only on change"]=0;
	p["format"]["File format"]="JPEG";
	p["resolution"]["Resolution of the image (most for raw video frames)"]=resolution_t{0,0};
	p["scan_total"]["Scan total number of files in the sequence (starts at 'index')"]=false;
	return p;
}


FilePicker::FilePicker(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::IOThread(log_,parent,1,1,std::string("file_picker")),
BasicEventConsumer(log),
BasicEventProducer(log),
index_(0),fps_(0.0),format_(0),
raw_format_(false),changed_(true),scan_total_(false)
{
	IOTHREAD_INIT(parameters)

	boost::regex pat("(.*)%(0?\\d+)d(.*)");
	boost::smatch what;
	if (!regex_search(pattern_.cbegin(), pattern_.cend(), what, pat, boost::match_default)) {
		throw exception::InitializationFailed("Failed to parse input pattern");
	}

	pattern_detail_.head = std::string (what[1].first, what[1].second);
	pattern_detail_.tail = std::string (what[3].first, what[3].second);
	auto counter = std::string (what[2].first, what[2].second);
	if (counter[0] == '0') {
		pattern_detail_.fill = true;
		pattern_detail_.counter = lexical_cast<index_t>(counter.substr(1));
	} else {
		pattern_detail_.fill = false;
		pattern_detail_.counter = lexical_cast<index_t>(counter);
	}
	log[log::info] << "Pattern: " << pattern_detail_.head << ", " << (pattern_detail_.fill?"fill with zeroes, ":"") << pattern_detail_.counter << " digits, " << pattern_detail_.tail;
}

FilePicker::~FilePicker() noexcept
{
}
namespace {
std::string get_filename(index_t index, pattern_detail_t detail)
{
	std::string filename = detail.head;
	auto cnt = lexical_cast<std::string>(index);
	if (detail.fill) {
		auto len = cnt.size();
		if (len < detail.counter) {
			filename += std::string(detail.counter - len, '0');
		}
	}
	filename += cnt + detail.tail;
	return filename;
}

core::pFrame read_file(format_t fmt, bool raw_format, resolution_t res, const std::string& filename)
{
	std::ifstream file(filename, std::ios::binary|std::ios::in);
	if (!file.is_open()) return {};
	file.seekg(0, std::ios::end);
	auto len = file.tellg();
	file.seekg(0, std::ios_base::beg);
	if (raw_format) {
		return {};
	}
	auto frame = core::CompressedVideoFrame::create_empty(fmt, res, len);
	file.read(reinterpret_cast<char*>(frame->get_data().data()), len);
	return frame;
}
bool test_file(const std::string& filename)
{
	std::ifstream file(filename, std::ios::binary|std::ios::in);
	return file.is_open();
}
}
void FilePicker::run()
{
	index_t max_index = 0;
	if (scan_total_) {
		auto idx = index_;
		while (test_file(get_filename(idx, pattern_detail_))) {
			++idx;
		}
		emit_event("max", idx);
		emit_event("last", idx>0?idx-1:0);
		emit_event("total", (idx-index_));
		max_index = 0;
	}

	raw_format_ = false;
	for (const auto& x: core::raw_format::formats()) {
		if (x.first == format_) {
			raw_format_ = true;
			break;
		}
	}

	auto frame_duration = fps_>0.0?1_s/fps_:0_s;
	auto next_time = timestamp_t{};
	while (still_running()) {
		wait_for_events(get_latency());
		process_events();
		if (fps_ > 0.0) {
			if (timestamp_t{} > next_time) {
				if (!max_index || index_ < (max_index-1)) {
					next_time = next_time+frame_duration;
					++index_;
					changed_ = true;
				}
			}
		}
		if (changed_) {
			emit_event("index",index_);
			std::string filename = get_filename(index_, pattern_detail_);
			if(auto frame = read_file(format_, raw_format_, resolution_, filename)) {
				log[log::debug] << "Read file " << filename;
				push_frame(0,std::move(frame));
			} else {
				log[log::warning] << "Failed to read file " << filename;
			}
			changed_ = false;
		}
	}

}
namespace {
format_t parse_format(const std::string& fmt_str)
{
	if (auto f = core::raw_format::parse_format(fmt_str)) {
		return f;
	}
	return core::compressed_frame::parse_format(fmt_str);
}
}
bool FilePicker::set_param(const core::Parameter& param)
{
	if (assign_parameters(param)
			(pattern_, "pattern")
			(index_, "index")
			(fps_, "fps")
			(resolution_, "resolution")
			(scan_total_, "scan_total")
			.parsed<std::string>
				(format_, "format", parse_format))
		return true;
	return core::IOThread::set_param(param);
}
bool FilePicker::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
	if (assign_events(event_name, event)
			(index_, "index")
			.bang("next", [this](){++index_;})
			.bang("prev", [this](){if (index_>0)--index_;})
			.bang("reload", [](){})) {
		changed_ = true;
		return true;
	}
	return false;
}

} /* namespace file_picker */
} /* namespace yuri */
