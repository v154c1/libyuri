/*!
 * @file 		Log.cpp
 * @author 		Zdenek Travnicek
 * @date 		24.7.2010
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2010 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "Log.h"
#include <map>
#include "yuri/core/utils.h"
#include "yuri/core/utils/string_generator.h"
namespace yuri
{
namespace log
{


std::atomic<int> Log::uids;

namespace {
const std::map<debug_flags_t, std::string> level_names= {
	{fatal,			"FATAL ERROR"},
	{error,			"ERROR"},
	{warning,		"WARNING"},
	{info,			"INFO"},
	{debug,			"DEBUG"},
	{verbose_debug,	"VERBOSE_DEBUG"},
	{trace,			"TRACE"}};

const std::map<debug_flags_t, std::string> level_colors = {
#if defined(YURI_LINUX) || defined(YURI_BSD)
	{fatal,			"\033[4;31;42m"}, // Red, underscore, bg
	{error,			"\033[31m"}, // Red
	{warning,		"\033[35m"},
	{info,			"\033[00m"},
	{debug,			"\033[35m"},
	{verbose_debug,	"\033[36m"},
	{trace,			"\033[4;36m"}
#else
	{fatal,			""},
	{error,			""},
	{warning,		""},
	{info,			""},
	{debug,			""},
	{verbose_debug,	""},
	{trace,			""}
#endif
};

long adjust_level_flag(long f, long delta)
{
	const auto level = f & flag_mask;
	const auto flags = f & (~flag_mask);
	const auto new_level = clip_value(level + delta, silent, trace);
	return new_level | flags;
}

template<class Stream>
void print_date_time(Stream& os)
{
	os << core::utils::generate_string("%lx ");
}

template<class Stream>
void print_date(Stream& os)
{
	os << core::utils::generate_string("%lT ");
}

template<class Stream>
void print_time(Stream& os)
{
	os << core::utils::generate_string("%lt ");
}


template<class Stream>
void print_level(Stream& os, debug_flags flags, const long output_flags)
{
	const auto f = static_cast<debug_flags>(flags&flag_mask);
	const auto it = level_names.find(f);
	if (it == level_names.end()) {
		return;
	}
	const auto& name = it->second;

	if (output_flags&use_colors) {
		auto it_col = level_colors.find(f);
		auto it_col2 = level_colors.find(info);
		if (it_col!=level_colors.end() && it_col2 != level_colors.end()) {
			const auto& color1 = it_col->second;
			const auto& color2 = it_col2->second;
			os <<  color1 << name << color2 << " ";
			return;
		}
	}
	os << name << " ";
}

}

/**
 * Creates a new Log instance
 * @param out Stream to output to
 */
Log::Log(std::ostream &out):uid(uids++),out(std::make_shared<guarded_stream<char>>(out)),
logger_name_(""),output_flags_(info|show_level),quiet_(false)
{
}


Log& Log::operator=(Log&& rhs) noexcept
{
	uid = rhs.uid;
	out = std::move(rhs.out);
	logger_name_ = std::move(rhs.logger_name_);
	output_flags_ = std::move(rhs.output_flags_);
	quiet_ = rhs.quiet_;
	rhs.quiet_ = true;
	return *this;
}

/**
 * Destroys an instance
 */
Log::~Log() noexcept
{
	if (out) (*this)[verbose_debug] << "Destroying logger " << uid;
}

/**
 * Creates a new Log instance as a copy of @em log.  The instance will get an unique uid.
 * @param log Log instance to copy from
 */
Log::Log(const Log &log):uid(uids++),out(log.out),logger_name_(""),
		output_flags_(log.output_flags_),quiet_(log.quiet_)
{
	(*this)[verbose_debug] << "Copying logger "	<< log.uid << " -> " << uid;
}
/**
 * Creates a new Log instance by moving from @em rhs.  The instance will retain uid of the original log object
 * @param log Log instance to move from
 */

Log::Log(Log&& rhs) noexcept
:uid(rhs.uid),out(std::move(rhs.out)),logger_name_(std::move(rhs.logger_name_)),
output_flags_(std::move(rhs.output_flags_)),quiet_(rhs.quiet_)
{
	rhs.quiet_ = true;
}
/**
 * Sets textual label for current instance
 * @param s new label
 */
void Log::set_label(std::string s)
{
	logger_name_ = std::move(s);
}

/**
 * Return an instance of LogProxy that should be used for current level. It should not outlive the original Log object!
 * @param f flag to compare with. If this flag represents log level higher than current, the LogProxy object will be 'dummy'
 * @return An instance of LogProxy
 */
LogProxy<char> Log::operator[](debug_flags f) const
{
	const bool dummy = f > (output_flags_ & flag_mask);
	if (!out) throw std::runtime_error("Invalid log object");
	LogProxy<char> lp(*out, dummy);
	if (!quiet_ && !dummy) {
		if (output_flags_&show_level) {
			print_level(lp, f, output_flags_);
		}
		lp <<  uid << ": ";
		if (output_flags_&show_date && output_flags_&show_time) {
			// Printing date and time together should be slightly faster than printing it separately.
			// It also prevent problems with inconsistencies.
			print_date_time(lp);
		} else {
			if (output_flags_&show_date) {
				print_date(lp);
			}
			if (output_flags_&show_time) {
				print_time(lp);
			}
		}
		lp << logger_name_;
	}
	return lp;
}


void Log::adjust_log_level(long delta)
{
	output_flags_ = adjust_level_flag(output_flags_, delta);
}


}
}
