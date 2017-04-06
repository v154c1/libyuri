/*!
 * @file 		string_generator.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		31. 3. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "string_generator.h"

#ifdef HAVE_BOOST_REGEX
#include <boost/regex.hpp>
#include "yuri/core/utils/hostname.h"
#include "yuri/version.h"
#include "yuri/core/utils/frame_info.h"
#include "yuri/core/utils/global_time.h"
#include "yuri/core/frame/Frame.h"
#include "yuri/core/frame/VideoFrame.h"
#include "yuri/core/utils/wall_time.h"
#endif

namespace yuri {
namespace core {
namespace utils {

#ifndef HAVE_BOOST_REGEX
bool is_extended_generator_supported()
{
	return false;
}

std::pair<bool, bool> analyze_string_specifiers(const std::string& /* pattern */)
{
	return std::make_pair(false, false);
}

std::string generate_string(const std::string& pattern, index_t /*  sequence */, const yuri::core::pFrame& /* frame */)
{
	return pattern;
}
#else

bool is_extended_generator_supported()
{
	return true;
}

namespace {

const boost::regex specifier_pattern ("%(0?\\d+[simMoeEcdz]|[simMoeEcdzntfFTrHDSOv%]|0?\\d?[lq][YMdDhHmstTx])");

template<class S1, class S2>
std::string to_s(const std::pair<S1, S2>& p)
{
	return std::string(p.first, p.second);
}

template<class Value>
std::string parse_and_replace(const std::string& spec, const Value& value)
{
	boost::regex pat ("%(0?)(\\d*)([lq]?)([[:alpha:]])");
	boost::smatch what;
	std::stringstream ss;
	if (boost::regex_match(spec, what, pat)) {
		const bool zero = what[1].first!=what[1].second;
		const auto count = what[2].first==what[2].second?
						0:std::stoul(to_s(what[2]));
		print_formated_value(ss, value, count, zero);
	}
	return ss.str();
}

template<class T, class T2>
char check_complex_specifier(std::pair<T, T2> w)
{
	const auto& start = w.first;
	const auto& end = w.second;
	if (std::distance(start, end) > 1) {
		auto c = *(end-2);
		switch (c) {
			case 'l':
				return c;
			case 'q':
				return c;
			default:
				break;
		}
	}
	return 0;
}

std::string generate_localtime(const std::string& spec, const std::tm& t)
{
	boost::regex pat ("%(0?)(\\d*)l([[:alpha:]])");
	boost::smatch what;
	std::stringstream ss;
	if (boost::regex_match(spec, what, pat)) {
		const bool zero = what[1].first!=what[1].second;
		const auto count = what[2].first==what[2].second?
						0:std::stoul(to_s(what[2]));
		int value = -1;

		switch(*(what[3].second-1)) {
		case 'Y':
			value = t.tm_year + 1900;
			break;
		case 'M':
			value = t.tm_mon + 1;
			break;
		case 'd':
			value = t.tm_mday;
			break;
		case 'D':
			value = t.tm_yday;
			break;
		case 'H':
			value = t.tm_hour;
			break;
		case 'h':
			value = t.tm_hour % 12;
			break;
		case 'm':
			value = t.tm_min;
			break;
		case 's':
			value = t.tm_sec;
			break;
		case 'T':
			ss << generate_localtime("%04lY",t) << "-" << generate_localtime("%02lM",t) << "-" << generate_localtime("%02ld",t);
			break;
		case 't':
			ss << generate_localtime("%02lH",t) << ":" << generate_localtime("%02lm",t) << ":" << generate_localtime("%02ls",t);
			break;
		case 'x':
			ss << generate_localtime("%lT",t) << " " << generate_localtime("%lt",t);
			break;
		default:
			break;
		}
		if (value >= 0) {
			print_formated_value(ss, value, count, zero);
		}
	}
	return ss.str();
}

std::string generate_localtime(const std::string& spec)
{
	auto t = get_current_local_time();
	return generate_localtime(spec, t);
}

std::string generate_startuptime(std::string spec)
{
	auto t = get_startup_local_time();
	// Change the specifier from 'q' to 'l' and let's reuse generate_localtime...
	if (auto idx = spec.find('q')) {
		spec[idx]='l';
	}

	return generate_localtime(spec, t);
}


}


std::pair<bool, bool> analyze_string_specifiers(const std::string& pattern)
{
	bool any_spec = false;
	bool seq_spec = false;
	auto beg = pattern.cbegin();
	auto end = pattern.cend();

	boost::smatch what;

	while(boost::regex_search(beg, end, what, specifier_pattern, boost::match_default)) {
		any_spec = true;

		assert (std::distance(what[0].first, what[0].second) > 0);
		auto specifier = *(what[0].second - 1);
		if (check_complex_specifier(what[0])) {
			seq_spec = true;
		} else {
			switch (specifier) {
				case 't':
				case 'T':
				case 's':
				case 'i':
				case 'f':
				case 'F':
				case 'm':
				case 'M':
				case 'o':
				case 'r':
				case 'z':
					seq_spec = true;
					break;
				default:
					break;
			}
		}

		if (seq_spec) break;

		beg = what[0].second;
	}
	return std::make_pair(any_spec, seq_spec);
}

std::string generate_string(const std::string& pattern, index_t sequence, const yuri::core::pFrame& frame)
{
	std::string new_str;
	auto beg = pattern.cbegin();
	auto end = pattern.cend();
	boost::smatch what;
	std::stringstream ss;
	const timestamp_t current_timestamp;
	while(boost::regex_search(beg, end, what, specifier_pattern, boost::match_default)) {
		assert (std::distance(what[0].first, what[0].second) > 0);
		if (beg != what[0].first) {
			ss << std::string{beg, what[0].first};
		}
		auto specifier = *(what[0].second - 1);
		if (auto c = check_complex_specifier(what[0])) {
			switch (c) {
				case 'l':
					ss << generate_localtime(to_s(what[0]));
					break;
				case 'q':
					ss << generate_startuptime(to_s(what[0]));
					break;
				default:
					break;
			}
		} else {
			switch (specifier) {
				case 's':
					ss << parse_and_replace(to_s(what[0]), sequence);
					break;
				case 'i':
					if (frame) {
						ss << parse_and_replace(to_s(what[0]), frame->get_index());
					}
					break;
	//			case 'n':
	//				ss << get_node_name();
	//				break;
				case 'T':
					ss << current_timestamp;
					break;
				case 't':
					if (frame) ss << frame->get_timestamp();
					break;
				case 'm':
					if (frame) ss << parse_and_replace(to_s(what[0]),
							(frame->get_timestamp() - core::utils::get_global_start_time()).value/1000);
					break;
				case 'M':
					if (frame) ss << parse_and_replace(to_s(what[0]),
							(frame->get_timestamp() - core::utils::get_global_start_time()).value);
					break;
				case 'o':
					if (frame) ss << parse_and_replace(to_s(what[0]),
							(frame->get_timestamp() - core::utils::get_global_start_time()).value/1000000);
					break;
				case 'e':
					if (frame) ss << parse_and_replace(to_s(what[0]),
							(timestamp_t{} - core::utils::get_global_start_time()).value/1000);
					break;
				case 'E':
					if (frame) ss << parse_and_replace(to_s(what[0]),
							(timestamp_t{} - core::utils::get_global_start_time()).value);
					break;
				case 'c':
					if (frame) ss << parse_and_replace(to_s(what[0]),
							(timestamp_t{} - core::utils::get_global_start_time()).value/1000000);
					break;
				case 'd':
					if (frame) ss << parse_and_replace(to_s(what[0]),
							(timestamp_t{} - frame->get_timestamp()).value/1000);
					break;
				case 'H':
					ss << core::utils::get_hostname();
					break;
				case 'D':
					ss << core::utils::get_domain();
					break;
				case 'S':
					ss << core::utils::get_sysver();
					break;
				case 'O':
					ss << core::utils::get_sysname();
					break;
				case 'v':
					ss << yuri_version;
					break;
				case 'f':
					if (frame) ss << core::utils::get_frame_type_name(frame->get_format(), true);
					break;
				case 'F':
					if (frame) ss << core::utils::get_frame_type_name(frame->get_format(), false);
					break;
				case 'r':
					if (auto f = std::dynamic_pointer_cast<core::VideoFrame>(frame))
						ss << f->get_resolution();
					break;
				case 'z':
					if (frame)
						ss << parse_and_replace(to_s(what[0]), frame->get_size());
					break;
				case '%':
					ss << "%";
					break;
				default:
					break;
			}
		}
		beg = what[0].second;
	}
	if (beg != end) {
		ss << std::string(beg, end);
	}
	return ss.str();
}
#endif

namespace {
const std::vector<string_generator_placeholder_info_t> specifier_list = {
		{"t",	"timestamp from the incomming frame", false},
		{"f",	"frame type (short)", false},
		{"F",	"frame type (long)", false},
		{"T",	"timestamp at the time of dump", false},
		{"r",	"resolution of video frame", false},
		{"z",	"size of frame in bytes", true},
		{"m",	"milliseconds since start (from frame timestamp)", true},
		{"M",	"microseconds since start (from frame timestamp)", true},
		{"o",	"seconds since start (from frame timestamp)", true},
		{"s",	"sequence number", true},
		{"i",	"frame index", true},
		{"e",	"milliseconds since start (current time)", true},
		{"E",	"microseconds since start (current time)", true},
		{"c",	"seconds since start (current time)", true},
		{"d",	"milliseconds as a difference of frame time and current time (== frame's actual lifetime)", true},

		{"lY",	"Current year (localtime)", false},
		{"lM",	"Current month (localtime)", false},
		{"ld",	"Current day in month (localtime)", false},
		{"lD",	"Current day of year (localtime)", false},
		{"lH",	"Current hour 0-23 (localtime)", false},
		{"lh",	"Current hour 0-11 (localtime)", false},
		{"lm",	"Current minute (localtime)", false},
		{"ls",	"Current second (localtime)", false},
		{"lT",	"Curent date (YYYY-MM-DD)", false},
		{"lt",	"Current time (HH:MM:SS)", false},
		{"lx",	"Current date time (YYY-MM-DD HH:MM:SS)", false},
		{"n",	"module name (currently unsupported)", false},
		{"H",	"local hostname", false},
		{"D",	"local domain", false},
		{"S",	"system name (eg. Linux-3.19.0)", false},
		{"O",	"OS (eg. Linux)", false},
		{"v",	"yuri version", false},
		{"%",	"literal %", false},
		{"qY",	"Start up year (localtime)", false},
		{"qM",	"Start up month (localtime)", false},
		{"qd",	"Start up day in month (localtime)", false},
		{"qD",	"Start up day of year (localtime)", false},
		{"qH",	"Start up hour 0-23 (localtime)", false},
		{"qh",	"Start up hour 0-11 (localtime)", false},
		{"qm",	"Start up minute (localtime)", false},
		{"qs",	"Start up second (localtime)", false},
		{"qT",	"Start up date (YYYY-MM-DD)", false},
		{"qt",	"Start up time (HH:MM:SS)", false},
		{"qx",	"Start up time (YYY-MM-DD HH:MM:SS)", false},


};
}


std::vector<string_generator_placeholder_info_t> enumerate_string_generator_specifiers() {
	return specifier_list;
}

}
}
}


