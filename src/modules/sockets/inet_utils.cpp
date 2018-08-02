/*
 * YuriInetSocket.cpp
 *
 *  Created on: 19. 2. 2015
 *      Author: neneko
 */
#include "inet_utils.h"
#include "yuri/core/utils.h"
#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <functional>

#include <iostream>
#if defined(__GNUC__) && (__GNUC__ >= 7)
#include <functional>
#endif

namespace yuri{
namespace network {

namespace inet {
namespace {
std::unique_ptr<addrinfo, std::function<void(addrinfo*)>>
get_addr_info(const std::string& server, uint16_t port, bool passive, int type, int domain = AF_UNSPEC)
{
	static const addrinfo hints = {passive?AI_PASSIVE:0, domain, type, 0, 0, nullptr, nullptr, nullptr};
	addrinfo *info = nullptr;
	const char* addr = nullptr;
	if (!server.empty()) addr = server.c_str();
	/* int ret = */ ::getaddrinfo(addr,
					lexical_cast<std::string>(port).c_str(),
	                       &hints,
	                       &info);
	return {info, [](addrinfo* p){freeaddrinfo(p);}};
}

sockaddr_in* get_inet_addr(sockaddr* addr)
{
	if (!addr || addr->sa_family != AF_INET) return nullptr;
	return reinterpret_cast<sockaddr_in*>(addr);
}

bool is_address_multicast(sockaddr_in* addr)
{
	if (!addr) return false;
	const auto iclass = addr->sin_addr.s_addr&0xFF;
	if (iclass >=224 && iclass <= 239) return true;
	return false;
}

sockaddr_in6* get_inet_addr6(sockaddr* addr)
{
	if (!addr || addr->sa_family != AF_INET6) return nullptr;
	return reinterpret_cast<sockaddr_in6*>(addr);
}

bool is_address_multicast6(sockaddr_in6* addr)
{
	if (!addr) return false;
	return IN6_IS_ADDR_MULTICAST((struct in6_addr *)(addr->sin6_addr.s6_addr));
}

bool register_multicast(YuriNetSocket& socket, sockaddr* addr)
{
	auto iaddr = get_inet_addr(addr);

	if (is_address_multicast(iaddr)) {
		u_char ttl = 2;
		ip_mreq mcast_req = {iaddr->sin_addr, {INADDR_ANY}};

		return (::setsockopt(socket.get_socket(), IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) == 0) &&
				(::setsockopt (socket.get_socket(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcast_req, sizeof(mcast_req)) == 0);
	}
	auto iaddr6 = get_inet_addr6(addr);
	if (is_address_multicast6(iaddr6)) {
		u_char ttl = 2;
		ipv6_mreq mcast_req = {iaddr6->sin6_addr, 0};
		return (::setsockopt(socket.get_socket(), IPPROTO_IP, IPV6_MULTICAST_HOPS, &ttl, sizeof(ttl)) == 0) 
// This seems not be implemented in FreeBSD 
#ifdef IPV6_ADD_MEMBERSHIP
&& (::setsockopt (socket.get_socket(), IPPROTO_IP, IPV6_ADD_MEMBERSHIP, &mcast_req, sizeof(mcast_req)) == 0)
#endif
;

	}
	return false;
}

}

bool bind(YuriNetSocket& socket, const std::string& address, uint16_t port)
{
	auto info = get_addr_info(address, port, true, socket.get_sock_type(), socket.get_sock_domain());
	if (!info) return false;
	register_multicast(socket, info->ai_addr);
	return ::bind(socket.get_socket(), info->ai_addr, info->ai_addrlen) == 0;
}

bool connect(YuriNetSocket& socket, const std::string& address, uint16_t port)
{
	auto info = get_addr_info(address, port, false, socket.get_sock_type(), socket.get_sock_domain());
	if (!info) return false;
	register_multicast(socket, info->ai_addr);
	return ::connect(socket.get_socket(), info->ai_addr, info->ai_addrlen) == 0;
}



}
}

}
