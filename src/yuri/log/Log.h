/*!
 * @file 		Log.h
 * @author 		Zdenek Travnicek
 * @date 		24.7.2010
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2010 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef LOG_H_
#define LOG_H_
#include "yuri/core/utils/new_types.h"
#include "LogUtils.h"
#include "LogProxy.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <atomic>

//! @namespace yuri All yuri related stuff belongs here
namespace yuri
{
//! @namespace yuri::log All logging related stuff belongs here
namespace log {

/**
 * @brief Main logging facility for libyuri
 */
class Log
{
public:
	//! Constructs Log instance with @em out as a backend
	EXPORT Log(std::ostream &out);
    //! Constuct Log instance with a custom stream output
    EXPORT Log(std::shared_ptr<generic_out_stream<char>> out);
	//! Constructs Log instance as a copy of @em log, with a new id
	EXPORT Log(const Log& log);
	EXPORT Log(Log&& log) noexcept;


	EXPORT Log& operator=(Log&& rhs) noexcept;
	EXPORT virtual ~Log() noexcept;
//	EXPORT void set_id(int id);
	EXPORT void set_label(std::string s);
	EXPORT void set_flags(long f) { output_flags_=f; }
	EXPORT LogProxy<char> operator[](debug_flags f) const;
	EXPORT long get_flags() { return output_flags_; }
	EXPORT void set_quiet(bool q) {quiet_ =q;}
	EXPORT void adjust_log_level(long delta);

private:
	// Global counter for IDs
	static std::atomic<int> uids;
	// ID of current Log
	int uid;
	// Output stream
	std::shared_ptr<generic_out_stream<char> > out;

	std::string logger_name_;
	long output_flags_;
	bool quiet_;

};

}
}
#endif /*LOG_H_*/
