/*
 * yuri_simple.cpp
 *
 *  Created on: 11. 1. 2015
 *      Author: neneko
 */

#include "simple/SimpleBuilder.h"
#include "yuri/exception/InitializationFailed.h"
#include "yuri/core/thread/FixedMemoryAllocator.h"
#include "yuri/core/utils/array_range.h"
#include <string>
#include <memory>


yuri::log::Log l(std::clog);
std::shared_ptr<yuri::simple::SimpleBuilder> builder;

#if defined YURI_POSIX
#include <signal.h>
#include <string.h>
static void sigHandler(int sig, siginfo_t *siginfo, void *context);
static struct sigaction act;


void sigHandler(int sig, siginfo_t */*siginfo*/, void */*context*/)
{
#if !defined YURI_APPLE && !defined YURI_BSD
	if (sig==SIGRTMIN) {
		l[yuri::log::warning] << "Realtime signal 0! Ignoring...";
		return;
	}
#endif
	if (sig==SIGPIPE) {
		// Sigpipe needs to be ignored, otherwise application may get killed randomly
		return;
	}
	if (builder)
		builder->request_end(yuri::core::yuri_exit_interrupted);
	act.sa_handler = SIG_DFL;
	act.sa_flags &= ~SA_SIGINFO;
	sigaction(SIGINT,&act,0);
}
#endif



int main(int argc, char** argv)
{
#if defined YURI_POSIX
	memset (&act, '\0', sizeof(act));
	act.sa_sigaction = &sigHandler;
	act.sa_flags = SA_SIGINFO;
    sigaction(SIGINT,&act,nullptr);
    sigaction(SIGTERM,&act,nullptr);
    sigaction(SIGPIPE,&act,nullptr);
#if !defined YURI_APPLE && !defined YURI_BSD
    sigaction(SIGRTMIN,&act,nullptr);
#endif
#endif

	std::vector<std::string> arguments;
	for (auto&& s: yuri::array_range<char*>(argv+1, argc-1)) {
		arguments.push_back(s);
	}

	l.set_flags(yuri::log::info|yuri::log::show_level|yuri::log::use_colors|yuri::log::show_time|yuri::log::show_date);
	int ret = 0;
	try {
		builder = std::make_shared<yuri::simple::SimpleBuilder> (l, yuri::core::pwThreadBase{}, arguments);
		(*builder)();
		l[yuri::log::info] << "Application successfully finished";
		builder.reset();
	}
	catch (yuri::exception::Exception &e) {
		l[yuri::log::fatal] << "Application failed to start: " << e.what();
		ret = 1;
	}
	catch(std::exception &e) {
		l[yuri::log::fatal] << "An error occurred during execution: " << e.what();
		ret = 1;
	}
	if (builder) builder.reset();
	auto mp = yuri::core::FixedMemoryAllocator::clear_all();
	l[yuri::log::info] << "Memory pool cleared ("<< mp.first << " blocks, " << mp.second << " bytes)";
	return ret;

}


