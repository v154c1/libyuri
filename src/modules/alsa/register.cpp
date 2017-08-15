/*
 * register.cpp
 *
 *  Created on: 28.10.2013
 *      Author: neneko
 */

#include "AlsaInput.h"
#include "AlsaOutput.h"
#include "yuri/core/thread/IOThreadGenerator.h"

namespace yuri {
namespace alsa {

MODULE_REGISTRATION_BEGIN("alsa")
	REGISTER_IOTHREAD("alsa_input",AlsaInput)
	REGISTER_IOTHREAD("alsa_output",AlsaOutput)
MODULE_REGISTRATION_END()

}
}
