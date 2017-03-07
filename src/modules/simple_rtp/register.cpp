/*!
 * @file 		register.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		6. 3. 2017
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "SimpleH264RtpSender.h"
#include "SimpleH264RtpReceiver.h"
#include "yuri/core/Module.h"

namespace yuri {
namespace simple_rtp {

MODULE_REGISTRATION_BEGIN("simple_rtp")
    REGISTER_IOTHREAD("simple_h264_sender", SimpleH264RtpSender)
    REGISTER_IOTHREAD("simple_h264_receiver", SimpleH264RtpReceiver)
MODULE_REGISTRATION_END()
}
}
