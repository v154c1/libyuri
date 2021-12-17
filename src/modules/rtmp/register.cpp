/*!
 * @file 		register.cpp
 * @author 		Jiri Melnikov
 * @date 		13.12.2021
 * @date		13.12.2021
 */

#include "RTMP.h"
#include "yuri/core/Module.h"

namespace yuri {

MODULE_REGISTRATION_BEGIN("rtmp")
    REGISTER_IOTHREAD("rtmp_output", rtmp::RTMP)
MODULE_REGISTRATION_END()

}
