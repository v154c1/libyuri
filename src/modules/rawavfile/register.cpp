/*!
 * @file 		register.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		4.3.2017
 * @copyright	Institute of Intermedia, 2017
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "RawAVFile.h"
#include "yuri/core/Module.h"
namespace yuri {

MODULE_REGISTRATION_BEGIN("avmodules")
	REGISTER_IOTHREAD("rawavsource",rawavfile::RawAVFile)
MODULE_REGISTRATION_END()


}



