/*!
 * @file 		hostname.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		19. 3. 2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef SRC_YURI_CORE_UTILS_HOSTNAME_H_
#define SRC_YURI_CORE_UTILS_HOSTNAME_H_

#include <string>
#include "platform.h"

namespace yuri {
namespace core {
namespace utils {

EXPORT std::string get_hostname();
EXPORT std::string get_domain();
EXPORT std::string get_sysname();
EXPORT std::string get_sysver();

}
}

}




#endif /* SRC_YURI_CORE_UTILS_HOSTNAME_H_ */
