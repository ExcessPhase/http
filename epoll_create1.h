#pragma once
#include <sys/epoll.h>
#include <unistd.h>
#include <system_error>
#include <cerrno>

namespace foelsche
{
namespace linux
{
struct epoll_create1
{	const int m_i;
	epoll_create1(const int flags)
		:m_i(::epoll_create1(flags))
	{	if (m_i == -1)
			throw std::system_error(std::error_code(errno, std::generic_category()), "epoll_create1() failed!");
	}
	~epoll_create1(void)
	{	close(m_i);
	}
};
}
}