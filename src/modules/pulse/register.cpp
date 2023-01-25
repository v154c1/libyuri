#include "PulseInput.h"
#include "PulseOutput.h"
#include "yuri/core/thread/IOThreadGenerator.h"
#include "yuri/core/thread/InputRegister.h"

namespace yuri {
namespace pulse {

MODULE_REGISTRATION_BEGIN("pulse")
	REGISTER_IOTHREAD("pulse_input",PulseInput)
	REGISTER_INPUT_THREAD("pulse_input", yuri::pulse::PulseInput::enumerate)
	REGISTER_IOTHREAD("pulse_output", PulseOutput)
	REGISTER_INPUT_THREAD("pulse_output", yuri::pulse::PulseOutput::enumerate)
MODULE_REGISTRATION_END()

}
}
