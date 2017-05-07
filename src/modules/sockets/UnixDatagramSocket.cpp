/*!
 * @file 		UnixDatagramSocket.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		25.01.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "UnixDatagramSocket.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>

namespace yuri {
namespace network {


UnixDatagramSocket::UnixDatagramSocket(const log::Log &log_, const std::string& s):
YuriDatagram(log_, s, AF_UNIX)
{
}

UnixDatagramSocket::~UnixDatagramSocket() noexcept
{
}

bool UnixDatagramSocket::do_bind(const std::string& address, uint16_t /* port */)
{
	sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	std::copy(address.cbegin(), address.cbegin() + std::min(address.size(), sizeof(addr.sun_path) - 1), addr.sun_path);
	return ::bind(get_socket(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
}

bool UnixDatagramSocket::do_connect(const std::string& address, uint16_t /* port */)
{
	sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	std::copy(address.cbegin(), address.cbegin() + std::min(address.size(), sizeof(addr.sun_path) - 1), addr.sun_path);
	return ::connect(get_socket(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
}


}
}


