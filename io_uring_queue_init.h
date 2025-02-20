#pragma once
#include <liburing.h>
#include <system_error>
#include <cerrno>
#include <new>
#include <set>
#include <memory>
#include "io_data.h"
#include "io_uring_wait_cqe.h"


namespace foelsche
{
namespace linux
{
struct io_data;
	/// a RAII wrapper for the ioring call of the same name
struct io_uring_queue_init
{	std::set<std::shared_ptr<io_data> > m_sIoData;
	struct io_uring m_sRing;
	io_uring_queue_init(const unsigned entries, const unsigned flags)
	{	const int i = ::io_uring_queue_init(entries, &m_sRing, flags);
		if (i < 0)
			throw std::system_error(std::error_code(-i, std::generic_category()), "io_uring_queue_init() failed!");
	}
	~io_uring_queue_init(void)
	{	for (const auto &pFirst : m_sIoData)
		{	const auto pSQE = io_uring_get_sqe(&m_sRing);
			io_uring_prep_cancel(pSQE, reinterpret_cast<void*>(pFirst.get()), 0);
			io_uring_submit(&m_sRing);
		}
		for (std::size_t i = 0, iMax = m_sIoData.size()*2; i < iMax; ++i)
		{	linux::io_uring_wait_cqe s(&m_sRing);
		}
		while (!m_sIoData.empty())
			m_sIoData.erase(m_sIoData.begin());
		io_uring_queue_exit(&m_sRing);
	}
	static struct io_uring_sqe *io_uring_get_sqe(struct io_uring *const ring)
	{	if (const auto p = ::io_uring_get_sqe(ring))
			return p;
		else
			throw std::bad_alloc();
	}
		/// factory methods for io-requests
	std::shared_ptr<io_data> createAccept(io_data::HANDLER, const std::shared_ptr<io_data_created_fd>&);
	std::shared_ptr<io_data> createRecv(io_data::HANDLER, const std::shared_ptr<io_data_created_fd>&);
	static int getServerSocket(const io_data&);
	static int getFD(io_data_created&);
};
}
}
