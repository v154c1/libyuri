/*
 * register.cpp
 */

#include "NDIInput.h"
#include "yuri/core/thread/IOThreadGenerator.h"
#include "yuri/core/thread/InputRegister.h"

MODULE_REGISTRATION_BEGIN("ndi")
	REGISTER_IOTHREAD("ndi_input",yuri::ndi::NDIInput)
	REGISTER_INPUT_THREAD("ndi_input", yuri::ndi::NDIInput::enumerate)
	//REGISTER_IOTHREAD("ndi_output",yuri::ndi::NDIOutput)
MODULE_REGISTRATION_END()
