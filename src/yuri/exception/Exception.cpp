/*!
 * @file 		Exception.cpp
 * @author 		Zdenek Travnicek
 * @date 		31.5.2008
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2008 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "Exception.h"

namespace yuri
{
namespace exception
{

Exception::Exception():reason("Generic exception")
{
}

Exception::Exception(std::string reason):reason(std::move(reason))
{

}
Exception::~Exception() noexcept
{
}

const char *Exception::what() const noexcept
{
	return (const char*) reason.c_str();
}

}
}

// End of File
