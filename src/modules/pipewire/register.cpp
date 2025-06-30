#include "PipewireOutput.h"
#include "PipewireInput.h"
#include "yuri/core/thread/IOThreadGenerator.h"
#include "yuri/core/thread/InputRegister.h"

namespace yuri {
namespace pipewire {

MODULE_REGISTRATION_BEGIN("pipewire")
    REGISTER_IOTHREAD("pipewire_input", PipewireInput)
    REGISTER_INPUT_THREAD("pipewire_input", yuri::pipewire::PipewireInput::enumerate)
    REGISTER_IOTHREAD("pipewire_output", PipewireOutput)
    REGISTER_INPUT_THREAD("pipewire_output", yuri::pipewire::PipewireOutput::enumerate)
MODULE_REGISTRATION_END()

}
}
