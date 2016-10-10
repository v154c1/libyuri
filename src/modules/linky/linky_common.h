/*!
 * @file 		linky_common.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		26. 9. 2016
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file
 * doc/LICENSE
 *
 */

#ifndef SRC_MODULES_LINKY_LINKY_COMMON_H_
#define SRC_MODULES_LINKY_LINKY_COMMON_H_

#include <string>

namespace yuri {
namespace linky {

std::string download_url(const std::string& url, const std::string& api_key);

std::string upload_json(const std::string& url, std::string data_in, const std::string& api_key);
}
}

#endif /* SRC_MODULES_LINKY_LINKY_COMMON_H_ */
