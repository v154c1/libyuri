/*!
 * @file 		urlencode.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		30. 11. 2016
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef SRC_MODULES_WEBSERVER_URLENCODE_H_
#define SRC_MODULES_WEBSERVER_URLENCODE_H_

#include <string>

namespace yuri {
namespace webserver {

/*!
 * Decodes URLencoded string (%-encoding)
 * @param str urlencoded string
 * @return Decoded string
 */
std::string decode_urlencoded(std::string str);

/*!
 * Decodes numeric HTML entities (&#XYZ;)
 * @param str input string
 * @return String with decoded entities
 */
std::string decode_html_entities(std::string str);
}
}

#endif /* SRC_MODULES_WEBSERVER_URLENCODE_H_ */
