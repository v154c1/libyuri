/*!
 * @file 		register.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		26. 9. 2016
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "../linky/LinkyOutput.h"
#include "../linky/LinkyInput.h"
#include "yuri/core/Module.h"

namespace yuri {
namespace linky {



MODULE_REGISTRATION_BEGIN("linky")
		REGISTER_IOTHREAD("linky_output",LinkyOutput)
		REGISTER_IOTHREAD("linky_input",LinkyInput)
MODULE_REGISTRATION_END()

}
}

