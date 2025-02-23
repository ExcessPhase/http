#pragma once
#include <liburing.h>
#include <memory>
#include <functional>
#include <vector>
namespace foelsche
{
namespace linux
{
struct io_uring_queue_init;
	/// an abstract wrapper for any resources created
	/// pointed to by a stdLLshared_ptr
struct io_data_created:std::enable_shared_from_this<io_data_created>
{	virtual ~io_data_created(void) = default;
};
	/// derived for a file descriptor created by an accept() request
struct io_data_created_fd:io_data_created
{	const int m_iID;
	const bool m_bClose;
	io_data_created_fd(const int _i, const bool _bClose = true)
		:m_iID(_i),
		m_bClose(_bClose)
	{
	}
	~io_data_created_fd(void)
	{	if (m_bClose)
			::close(m_iID);
	}
};
	/// a buffer used for a read() request
struct io_data_created_buffer:io_data_created
{	std::vector<char> m_s;
	std::size_t m_iOffset;
	io_data_created_buffer(std::vector<char> _s, const std::size_t _iOffset = 0)
		:m_s(std::move(_s)),
		m_iOffset(_iOffset)
	{
	}
};
	/// the C++ base type of a request
struct io_data:std::enable_shared_from_this<io_data>
{		/// the type of a handler
	typedef std::function<void(io_uring_queue_init*const ring, ::io_uring_cqe* const cqe, const std::shared_ptr<io_data_created>&, io_data&) > HANDLER;
		/// the stored handler
	const HANDLER m_sHandler;
	const std::shared_ptr<io_data_created> m_sData;
	static std::size_t s_iNextId;
	const std::size_t m_iId = s_iNextId++;
	io_data(HANDLER _s, const std::shared_ptr<io_data_created> &_sData)
		:m_sHandler(std::move(_s)),
		m_sData(_sData)
	{
	}
	virtual ~io_data(void) = default;
	virtual std::shared_ptr<io_data_created> getResource(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE) = 0;
	void handleW(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE);
	virtual int getFD(void) const = 0;
	static std::size_t getSendOffset(const io_data&);
	static std::shared_ptr<io_data_created_buffer> getSendBuffer(const io_data&);
};
}
}
