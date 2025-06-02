#include "PipewireOutput.h"
#include "yuri/core/thread/IOThreadGenerator.h"
#include "yuri/core/thread/InputRegister.h"

namespace yuri {
namespace pipewire {

MODULE_REGISTRATION_BEGIN("pipewire")
    REGISTER_IOTHREAD("pipewire_output", PipewireOutput)
MODULE_REGISTRATION_END()

}
}
