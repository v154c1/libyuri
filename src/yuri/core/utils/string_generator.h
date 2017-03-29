/*!
 * @file 		string_generator.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		31. 3. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef SRC_YURI_CORE_UTILS_STRING_GENERATOR_H_
#define SRC_YURI_CORE_UTILS_STRING_GENERATOR_H_
#include "yuri/core/forward.h"
#include <iomanip>
#include <vector>
namespace yuri {
namespace core {
namespace utils {


/*
 * If compiled with boost::regex support, then following format specifiers
 * are supported inside of the string:
 *
 * 	%t - timestamp from the incomming frame
 * 	%f - frame type (short)
 * 	%F - frame type (long)
 * 	%T - timestamp at the time of dump
 * 	%r - resolution of video frame
 * 	%z - size of frame in bytes
 * 	%m - (*) milliseconds since start (from frame timestamp)
 * 	%M - (*) microseconds since start (from frame timestamp)
 * 	%o - (*) seconds since start (from frame timestamp)
 * 	%s - (*) sequence number
 * 	%i - (*) frame index
 * 	%e - (*) milliseconds since start (current time)
 * 	%E - (*) microseconds since start (current time)
 * 	%c - (*) seconds since start (current time)
 * 	%d - (*) milliseconds as a difference of frame time and current time (== frame's actual lifetime)
 *
 * Complex specifiers
 *
 * 	%lY - Current year (localtime)
 * 	%lM - Current month (localtime)
 * 	%ld - Current day in month (localtime)
 * 	%lD - Current day of year (localtime)
 * 	%lH - Current hour 0-23 (localtime)
 * 	%lh - Current hour 0-11 (localtime)
 * 	%lm - Current minute (localtime)
 * 	%ls - Current second (localtime)
 * 	%lT - Curent date (YYYY-MM-DD)
 * 	%lt - Current time (HH:MM:SS)
 * 	%lx - Current date time (YYY-MM-DD HH:MM:SS)
 *
 * 	These specifiers are constant and should evaluate to the same value during the whole run of application
 *
 * 	%n - module name (currently unsupported)
 * 	%H - local hostname
 * 	%D - local domain
 * 	%S - system name (eg. Linux-3.19.0)
 * 	%O - OS (eg. Linux)
 * 	%v - yuri version
 * 	%% - literal %
 *
 * 	%qY - Start up year (localtime)
 * 	%qM - Start up month (localtime)
 * 	%qd - Start up day in month (localtime)
 * 	%qD - Start up day of year (localtime)
 * 	%qH - Start up hour 0-23 (localtime)
 * 	%qh - Start up hour 0-11 (localtime)
 * 	%qm - Start up minute (localtime)
 * 	%qs - Start up second (localtime)
 * 	%qT - Start up date (YYYY-MM-DD)
 * 	%qt - Start up time (HH:MM:SS)
 * 	%qx - Start up time (YYY-MM-DD HH:MM:SS)


 *  Specifiers marked with (*) can also use syntax %[[0]width]X,
 *  where X is the specifier, width is minimal width of the output and
 *  optional zero at the beginning adds padding with literal '0'.
 *  e.g. (for frame with index 123):
 *  %i   -> '123'
 *  %7i  -> '    123'
 *  %05i -> '00123'
 *  %02i -> '123'
 *
 * Without boost::regex support, only a subset of the values is supported.
 */


/* **************************************************
 *  NOTE: The API for string generator is by no means stable
 *  and it can change anytime!
 ************************************************** */

/*!
 *
 * @return true if extended string generator is supported
 */
bool is_extended_generator_supported();

/*!
 * Analyzes pattern and determined, if there are any specifiers at all
 * and if there are any non-constant specifiers (changing with frame, time, ...)
 * @param pattern String containing the pattern
 * @return a pair of bools, where first value is true if there are any specifier,
 * 		second if there are any non-constant specifiers.
 */
std::pair<bool, bool> analyze_string_specifiers(const std::string& pattern);


/*!
 * Generates a string by replacing the specifiers in the pattern with values
 * from system and from (optionaly) provided frame.
 * @param pattern Input pattern string
 * @param sequence sequence number of current frame
 * @param frame A frame used for frame specific replacements. Can be empty
 * @return String with the specifiers replaced by actual values.
 */
std::string generate_string(const std::string& pattern, index_t sequence = 0, const yuri::core::pFrame& frame = {});

/*!
 * Simple wrapper for setw and setfill, allowing to simplify outputting of numbers.
 * @param os Output stream
 * @param value value to print
 * @param width Minimal number of digits the nubmer should take
 * @param zero Set to true to prepend '0's.
 * @return reference to output stream
 */
template<class Stream, class Value>
Stream& print_formated_value(Stream& os, const Value& value, int width = 0, bool zero = true)
{
	if (zero) {
		os << std::setfill('0');
	}
	if (width > 0) {
		os << std::setw(width);
	}
	os << value << std::setw(0);
	return os;
}


struct string_generator_placeholder_info_t {
	std::string placeholder;
	std::string description;
	bool accepts_width;
};

std::vector<string_generator_placeholder_info_t> enumerate_string_generator_specifiers();

}
}
}



#endif /* SRC_YURI_CORE_UTILS_STRING_GENERATOR_H_ */
