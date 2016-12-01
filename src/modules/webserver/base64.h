/*!
 * @file 		base64.h
 * @author 		Zdenek Travnicek <travnicek@cesnet.cz>
 * @date		08.12.2014
 * @copyright	CESNET, z.s.p.o, 2014
 * 				Distributed under modified BSD or GPL License,
 * 				see /doc/LICENSE.txt for details
 *
 */

#ifndef SRC_MODULES_WEBSERVER_BASE64_H_
#define SRC_MODULES_WEBSERVER_BASE64_H_

#include <string>

namespace yuri {
namespace webserver {
namespace base64 {
std::string encode(std::string);
std::string decode(const std::string&);
}
}
}

#endif /* SRC_MODULES_WEBSERVER_BASE64_H_ */
