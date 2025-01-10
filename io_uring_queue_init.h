#pragma once
#include <liburing.h>
#include <system_error>
#include <cerrno>
#include <new>


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
//struct io_uring_sqe *io_uring_get_sqe(struct io_uring *ring);
	static struct io_uring_sqe *io_uring_get_sqe(struct io_uring *const ring)
	{	const auto p = ::io_uring_get_sqe(ring);
		if (!p)
			throw std::bad_alloc();
	}
};
}
}
