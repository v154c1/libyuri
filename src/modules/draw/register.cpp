/*!
 * @file 		register.cpp
 */

#include "Box.h"
#include "yuri/core/Module.h"

namespace yuri {
namespace draw {

MODULE_REGISTRATION_BEGIN("draw")
		REGISTER_IOTHREAD("box", Box)
MODULE_REGISTRATION_END()

}
}

