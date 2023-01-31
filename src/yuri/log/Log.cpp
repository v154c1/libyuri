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


long adjust_level_flag(long f, long delta)
{
	const auto level = f & flag_mask;
	const auto flags = f & (~flag_mask);
	const auto new_level = clip_value(level + delta, silent, trace);
	return new_level | flags;
}



std::string generate_date_time()
{
    return core::utils::generate_string("%lx ");
}

std::string generate_date()
{
    return core::utils::generate_string("%lT ");
}

std::string generate_time()
{
    return core::utils::generate_string("%lt ");
}


/**
 * Creates a new Log instance
 * @param out Stream to output to
 */
Log::Log(std::ostream &out):uid(uids++),out(std::make_shared<guarded_stream<char>>(out)),
logger_name_(""),output_flags_(info|show_level),quiet_(false)
{
}

Log::Log(std::shared_ptr<generic_out_stream<char>> out):uid(uids++),out(std::move(out)),
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
    if (dummy) {
        return {*out, std::nullopt};
    }
	if (!quiet_) {
        LogMessageInfo info(uid, f, output_flags_, logger_name_);
		if (output_flags_&show_date && output_flags_&show_time) {
            info.time = generate_date_time();
		} else {
			if (output_flags_&show_date) {
                info.time = generate_date();
			}
			if (output_flags_&show_time) {
                info.time = generate_time();
			}
		}
        return {*out, std::move(info)};

	}
    return {*out,  LogMessageInfo{uid, f, 0, {}}};
}


void Log::adjust_log_level(long delta)
{
	output_flags_ = adjust_level_flag(output_flags_, delta);
}

}
}
