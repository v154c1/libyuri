/*!
 * @file 		UVUdpSocket.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		15.10.2013
 * @copyright	CESNET, z.s.p.o, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "UVUdpSocket.h"
//include "yuri/core/Module.h"
#include "yuri/core/socket/DatagramSocketGenerator.h"


namespace yuri {
namespace uv_udp {




UVUdpSocket::UVUdpSocket(const log::Log &log_, const std::string& url):
core::socket::DatagramSocket(log_,url),socket_(nullptr,[](socket_udp*s){if(s){udp_exit(s);}})
{
}

UVUdpSocket::~UVUdpSocket() noexcept
{
}


size_t UVUdpSocket::do_send_datagram(const uint8_t* data, size_t size)
{
	// !TODO Should be fixed upstream for const correctness) and const_cast removed!
	if (socket_.get()==nullptr) throw core::socket::socket_not_connected();
	return udp_send(socket_.get(),
			const_cast<char*>(reinterpret_cast<const char*>(data)),
			static_cast<int>(size));
}

size_t UVUdpSocket::do_receive_datagram(uint8_t* data, size_t size) {
	if (socket_.get()==nullptr) throw core::socket::socket_not_connected();
	return udp_recv(socket_.get(),
			reinterpret_cast<char*>(data),
			static_cast<int>(size));
}

bool UVUdpSocket::do_bind(const std::string& url, core::socket::port_t port) {
	if (url.empty()) {
		socket_.reset(udp_init("0.0.0.0",port,0,255,false, false));
	} else {
		socket_.reset(udp_init(url.c_str()/*nullptr*/,port,0,255,false, false));
	}
	return socket_.get();
}

bool UVUdpSocket::do_connect(const std::string& url, core::socket::port_t port) {
	if (url.empty()) {
		socket_.reset(udp_init("0.0.0.0",0,port,255,false,false));
	} else {
		socket_.reset(udp_init(url.c_str()/*nullptr*/,0,port,255,false,false));
	}
	return socket_.get();
}

bool UVUdpSocket::do_data_available() {
	if (socket_.get()==nullptr) throw core::socket::socket_not_connected();
	char c;
	return udp_peek(socket_.get(),&c, 1)!=0;
}

bool UVUdpSocket::do_ready_to_send() {
	return true; // whatever ;)
}

bool UVUdpSocket::do_wait_for_data(duration_t duration) {
	udp_fd_r fdr;
	udp_fd_set_r(socket_.get(), &fdr);
	timeval timeout = {duration.value/1000000, duration.value%1000000};
	bool ready = udp_select_r(&timeout, &fdr)>0;
	udp_fd_zero_r(&fdr);
	return ready; // TODO: Implement this....
}



} /* namespace uv_udp */
} /* namespace yuri */
