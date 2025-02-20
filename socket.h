#pragma once
#include <system_error>
#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace foelsche
{
namespace linux
{
	/// an RAII wrapper for socket()/close()
struct socket
{	const int m_i;
	socket(const int domain, const int type, const int protocol)
		:m_i(::socket(domain, type, protocol))
	{	if (m_i == -1)
			throw std::system_error(std::error_code(errno, std::generic_category()), "socket() failed!");
	}
	~socket(void)
	{	close(m_i);
	}
	static void bind(const int sockfd, const struct sockaddr *const addr, const socklen_t addrlen)
	{	if (::bind(sockfd, addr, addrlen) == -1)
			throw std::system_error(std::error_code(errno, std::generic_category()), "bind() failed!");
	}
	static void listen(const int sockfd, const int backlog)
	{	if (::listen(sockfd, backlog) == -1)
			throw std::system_error(std::error_code(errno, std::generic_category()), "listen() failed!");
	}
};
}
}
