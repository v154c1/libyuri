/*
 * register.cpp
 */

#include "OmxilOutput.h"
#include "yuri/core/thread/IOThreadGenerator.h"

MODULE_REGISTRATION_BEGIN("omxil")
	REGISTER_IOTHREAD("omxil_output",yuri::omxil::OmxilOutput)
MODULE_REGISTRATION_END()
