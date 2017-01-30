/*!
 * @file 		FileDump.h
 * @author 		Zdenek Travnicek
 * @date 		24.7.2010
 * @date		04.2.2014
 * @copyright	Institute of Intermedia, CTU in Prague, 2010 - 2014
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef FILEDUMP_H_
#define FILEDUMP_H_
#include "yuri/core/thread/IOFilter.h"
#include "yuri/event/BasicEventProducer.h"
#include "yuri/event/BasicEventConsumer.h"
#include <fstream>
#include <string>

namespace yuri
{
namespace dump
{


class FileDump: public core::IOFilter, public event::BasicEventProducer, public event::BasicEventConsumer
{
public:
	FileDump(log::Log &log_,core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~FileDump() noexcept;
	//static core::pIOThread generate(log::Log &_log,core::pwThreadBase parent, core::Parameters& parameters);
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
private:
	bool open_file(const std::string& fname);
	virtual core::pFrame do_simple_single_step(core::pFrame frame) override;
	virtual bool set_param(const core::Parameter &param) override;
	std::string generate_filename(const core::pFrame& frame);
	bool do_process_event(const std::string& event_name, const event::pBasicEvent& event);
	std::ofstream dump_file;
	std::string filename;

	int seq_chars;
	size_t seq_number, dumped_frames, dump_limit;


	bool use_regex_; //!< Filename contains format specifiers
	bool single_file_; //!< Output is in a single file

	bool append_;

	std::string info_string_;
};

}
}
#endif /*FILEDUMP_H_*/

