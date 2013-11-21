/*!
 * @file 		StreamSocketGenerator.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		15.10.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef STREAMSOCKETGENERATOR_H_
#define STREAMSOCKETGENERATOR_H_
#include "yuri/core/utils/BasicGenerator.h"
#include "yuri/core/utils/Singleton.h"
#include "StreamSocket.h"

namespace yuri {
namespace core{
template<
	class T,
	class KeyType>
class BasicStreamSocketGenerator: public core::BasicGenerator<T, KeyType,
		std::string,
		core::generator::DefaultErrorPolicy,
		function<shared_ptr<T> (yuri::log::Log &)>,
		function<void(void)>>
{
public:
	BasicStreamSocketGenerator(){}
};

typedef utils::Singleton<BasicStreamSocketGenerator<core::socket::StreamSocket, std::string>> StreamSocketGenerator;

#ifdef YURI_MODULE_IN_TREE
#define REGISTER_STREAM_SOCKET(name, type) namespace { bool reg_ ## type = yuri::core::StreamSocketGenerator::get_instance().register_generator(name,[](const yuri::log::Log& log){return make_shared<type>(log);}, function<void(void)>()); }
#else
#define REGISTER_STREAM_SOCKET(name, type) /*bool socket_reg_ ## type = */yuri::core::StreamSocketGenerator::get_instance().register_generator(name,[](const yuri::log::Log& log){return make_shared<type>(log);}, function<void(void)>());
#endif



}
}

#endif /* STREAMSOCKETGENERATOR_H_ */
