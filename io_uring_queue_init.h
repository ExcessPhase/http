#pragma once
#include <liburing.h>
#include <system_error>
#include <cerrno>


namespace foelsche
{
namespace linux
{
struct io_uring_queue_init
{	struct io_uring m_sRing;
	io_uring_queue_init(const unsigned entries, const unsigned flags)
	{	const int i = ::io_uring_queue_init(entries, &m_sRing, flags);
		if (i < 0)
			throw std::system_error(std::error_code(-i, std::generic_category()), "io_uring_queue_init() failed!");
	}
	~io_uring_queue_init(void)
	{	io_uring_queue_exit(&m_sRing);
	}
//int io_uring_queue_init(unsigned entries, struct io_uring *ring, unsigned flags);
};
}
}
