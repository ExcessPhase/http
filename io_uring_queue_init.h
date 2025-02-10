#pragma once
#include <liburing.h>
#include <system_error>
#include <cerrno>
#include <new>
#include <set>
#include <memory>
#include "io_data.h"


namespace foelsche
{
namespace linux
{
struct io_data;
struct io_uring_queue_init
{	std::set<std::shared_ptr<io_data> > m_sIoData;
	struct io_uring m_sRing;
	io_uring_queue_init(const unsigned entries, const unsigned flags)
	{	const int i = ::io_uring_queue_init(entries, &m_sRing, flags);
		if (i < 0)
			throw std::system_error(std::error_code(-i, std::generic_category()), "io_uring_queue_init() failed!");
	}
	~io_uring_queue_init(void)
	{	io_uring_queue_exit(&m_sRing);
	}
	static struct io_uring_sqe *io_uring_get_sqe(struct io_uring *const ring)
	{	if (const auto p = ::io_uring_get_sqe(ring))
			return p;
		else
			throw std::bad_alloc();
	}
	std::shared_ptr<io_data> createAccept(const int, io_data::HANDLER, std::shared_ptr<io_data_created>);
	std::shared_ptr<io_data> createRecv(const int, io_data::HANDLER, std::shared_ptr<io_data_created>);
	static int getServerSocket(const io_data&);
	static int getFD(io_data_created&);
};
}
}
