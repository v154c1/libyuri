/*!
 * @file 		register.cpp
 * @author 		Jiri Melnikov
 * @date 		13.12.2021
 * @date		13.12.2021
 */

#include "AVOutput.h"
#include "yuri/core/Module.h"

namespace yuri {

MODULE_REGISTRATION_BEGIN("avoutput")
    REGISTER_IOTHREAD("av_output", avoutput::AVOutput)
MODULE_REGISTRATION_END()

}
