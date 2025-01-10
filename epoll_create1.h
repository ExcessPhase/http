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
	static void epoll_ctl(const int epfd, const int op, const int fd, struct epoll_event *const event)
	{	if (::epoll_ctl(epfd, op, fd, event) == -1)
			throw std::system_error(std::error_code(errno, std::generic_category()), "epoll_ctl() failed!");
	}
	static int epoll_wait(const int epfd, struct epoll_event *const events, const int maxevents, const int timeout)
	{	const int i = ::epoll_wait(epfd, events, maxevents, timeout);
		if (i == -1)
			throw std::system_error(std::error_code(errno, std::generic_category()), "epoll_wait() failed!");
		else
			return i;
	}
//int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
//epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
};
}
}
