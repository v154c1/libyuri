/*!
 * @file 		UnixStreamSocket.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		25.01.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2015
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "UnixStreamSocket.h"
#include "yuri/core/utils.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace yuri {
namespace network {


UnixStreamSocket::UnixStreamSocket(const log::Log &log_):
YuriStreamSocket(log_, AF_UNIX)
{
}
UnixStreamSocket::UnixStreamSocket(const log::Log &log_, int sock):
YuriStreamSocket(log_, AF_UNIX, sock)
{

}
UnixStreamSocket::~UnixStreamSocket() noexcept
{
}

bool UnixStreamSocket::do_bind(const std::string& address, uint16_t /* port */)
{
	log[log::info] << "Binding to " << address;
	sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	std::copy(address.cbegin(), address.cbegin() + std::min(address.size(), sizeof(addr.sun_path) - 1), addr.sun_path);
	return ::bind(get_socket(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
}

bool UnixStreamSocket::do_connect(const std::string& address, uint16_t /* port */)
{
	sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	std::copy(address.cbegin(), address.cbegin() + std::min(address.size(), sizeof(addr.sun_path) - 1), addr.sun_path);
	return ::connect(get_socket(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
}
core::socket::pStreamSocket UnixStreamSocket::prepare_new(int sock_raw)
{
	if (sock_raw > 0) return std::make_shared<UnixStreamSocket>(log, sock_raw);
	return {};
}

} /* namespace yuri_tcp */
} /* namespace yuri */

