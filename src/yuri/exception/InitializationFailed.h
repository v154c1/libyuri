/*!
 * @file 		InitializationFailed.h
 * @author 		Zdenek Travnicek
 * @date 		28.7.2010
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2010 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef INITIALIZATIONFAILED_H_
#define INITIALIZATIONFAILED_H_

#include "Exception.h"
#include <string>
namespace yuri {

namespace exception {

class InitializationFailed: public yuri::exception::Exception {
public:
	EXPORT explicit InitializationFailed(const std::string& reason = "Failed to initialize object");
	EXPORT ~InitializationFailed() noexcept override = default;
	EXPORT InitializationFailed(const InitializationFailed&) noexcept = default;

};

}

}

#endif /* INITIALIZATIONFAILED_H_ */
