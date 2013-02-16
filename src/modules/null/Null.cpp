/*!
 * @file 		Null.cpp
 * @author 		Zdenek Travnicek
 * @date 		24.7.2010
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, 2010 - 2013
 * 				Distributed under GNU Public License 3.0
 *
 */

#include "Null.h"

namespace yuri
{

namespace io
{

REGISTER("null",Null)
IO_THREAD_GENERATOR(Null)

shared_ptr<Parameters> Null::configure()
{
	shared_ptr<Parameters> p = BasicIOThread::configure();
	return p;
}

Null::Null(Log &_log,pThreadBase parent, Parameters& parameters) IO_THREAD_CONSTRUCTOR
		:BasicIOThread(_log,parent,1,0,"Null")
{
	IO_THREAD_INIT("Null")
	latency=100000;
}

Null::~Null()
{
}

bool Null::step()
{
	int i=0;
	if (in[0]) while (in[0]->pop_frame()) ++i;
	if (i) log[verbose_debug] << "Deleted " << i << " frames" << std::endl;
	return true;
}


}

}
