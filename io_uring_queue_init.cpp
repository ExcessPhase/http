#include "io_uring_queue_init.h"
#include <netinet/in.h>
#include <fcntl.h>


namespace foelsche
{
namespace linux
{
namespace
{
	/// an accept request
struct io_data_accept:io_data
{	struct sockaddr_in client_addr;
	socklen_t client_len;
	io_data_accept(io_uring_queue_init *const _pRing, HANDLER _sHandler, const std::shared_ptr<io_data_created_fd> &_sData)
		:io_data(std::move(_sHandler), _sData),
		client_len(sizeof(struct sockaddr_in))
	{	io_uring_sqe* const sqe = io_uring_queue_init::io_uring_get_sqe(&_pRing->m_sRing);
		sqe->user_data = reinterpret_cast<uintptr_t>(this);
		io_uring_prep_accept(
			sqe,
			std::dynamic_pointer_cast<io_data_created_fd>(m_sData)->m_iID,
			reinterpret_cast<struct sockaddr*>(&client_addr),
			&client_len,
			0
		);
		io_uring_sqe_set_data(sqe, this);
		io_uring_submit(&_pRing->m_sRing);
	}
	virtual std::shared_ptr<io_data_created> getResource(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE) override
	{	return std::make_shared<io_data_created_fd>(_pCQE->res);
	}
	virtual int getFD(void) const override
	{	return std::dynamic_pointer_cast<io_data_created_fd>(m_sData)->m_iID;
	}
};
	/// a read request (from file)
struct io_data_read:io_data
{	const std::shared_ptr<io_data_created_buffer> m_sBuffer;
	io_data_read(
		io_uring_queue_init *const _pRing,
		HANDLER _sHandler,
		const std::shared_ptr<io_data_created_fd> &_rFD,
		const std::shared_ptr<io_data_created_buffer> &_rBuffer
	)
		:io_data(std::move(_sHandler), _rFD),
		m_sBuffer(_rBuffer)
	{	const auto sFD = std::dynamic_pointer_cast<io_data_created_fd>(m_sData);
		fcntl(sFD->m_iID, F_SETFL, fcntl(sFD->m_iID, F_GETFL, 0) | O_NONBLOCK);
		io_uring_sqe* const sqe = io_uring_queue_init::io_uring_get_sqe(&_pRing->m_sRing);
		sqe->user_data = reinterpret_cast<uintptr_t>(this);
		io_uring_prep_read(sqe, sFD->m_iID, m_sBuffer->m_s.data() + m_sBuffer->m_iOffset, m_sBuffer->m_s.size() - m_sBuffer->m_iOffset, 0);
		io_uring_sqe_set_data(sqe, this);
		io_uring_submit(&_pRing->m_sRing);
	}
	virtual std::shared_ptr<io_data_created> getResource(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE) override
	{	return m_sBuffer;
	}
	virtual int getFD(void) const override
	{	return std::dynamic_pointer_cast<io_data_created_fd>(m_sData)->m_iID;
	}
};
	/// a receive request (from socket)
struct io_data_recv:io_data
{	const std::shared_ptr<io_data_created_buffer> m_sBuffer;
	io_data_recv(
		io_uring_queue_init *const _pRing,
		HANDLER _sHandler,
		const std::shared_ptr<io_data_created_fd> &_rFD,
		const std::shared_ptr<io_data_created_buffer> &_rBuffer
	)
		:io_data(std::move(_sHandler), _rFD),
		m_sBuffer(_rBuffer)
	{	const auto sFD = std::dynamic_pointer_cast<io_data_created_fd>(m_sData);
		fcntl(sFD->m_iID, F_SETFL, fcntl(sFD->m_iID, F_GETFL, 0) | O_NONBLOCK);
		io_uring_sqe* const sqe = io_uring_queue_init::io_uring_get_sqe(&_pRing->m_sRing);
		sqe->user_data = reinterpret_cast<uintptr_t>(this);
		io_uring_prep_recv(sqe, sFD->m_iID, m_sBuffer->m_s.data() + m_sBuffer->m_iOffset, m_sBuffer->m_s.size() - m_sBuffer->m_iOffset, 0);
		io_uring_sqe_set_data(sqe, this);
		io_uring_submit(&_pRing->m_sRing);
	}
	virtual std::shared_ptr<io_data_created> getResource(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE) override
	{	return m_sBuffer;
	}
	virtual int getFD(void) const override
	{	return std::dynamic_pointer_cast<io_data_created_fd>(m_sData)->m_iID;
	}
};
struct io_data_write:io_data
{	const std::shared_ptr<io_data_created_buffer> m_sBuffer;
	io_data_write(
		io_uring_queue_init *const _pRing,
		HANDLER _sHandler,
		const std::shared_ptr<io_data_created_fd> &_rFD,
		const std::shared_ptr<io_data_created_buffer>&_rData
	)
		:io_data(
			std::move(_sHandler),
			_rFD
		),
		m_sBuffer(_rData)
	{
		io_uring_sqe* const sqe = io_uring_queue_init::io_uring_get_sqe(&_pRing->m_sRing);
		sqe->user_data = reinterpret_cast<uintptr_t>(this);
		io_uring_prep_write(
			sqe,
			_rFD->m_iID,
			_rData->m_s.data() + _rData->m_iOffset,
			_rData->m_s.size() - _rData->m_iOffset,
			0
		);
		io_uring_sqe_set_data(sqe, this);
		io_uring_submit(&_pRing->m_sRing);
	}
	virtual std::shared_ptr<io_data_created> getResource(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE) override
	{	return nullptr;
	}
	virtual int getFD(void) const override
	{	return std::dynamic_pointer_cast<io_data_created_fd>(m_sData)->m_iID;
	}
};
}
std::size_t io_data::getWriteOffset(const io_data&_r)
{	return dynamic_cast<const io_data_write&>(_r).m_sBuffer->m_iOffset;
}
std::shared_ptr<io_data_created_buffer> io_data::getWriteBuffer(const io_data&_r)
{	return dynamic_cast<const io_data_write&>(_r).m_sBuffer;
}
	/// create an accept request
std::shared_ptr<io_data> io_uring_queue_init::createAccept(io_data::HANDLER _sHandler, const std::shared_ptr<io_data_created_fd> &_sData)
{	return *m_sIoData.insert(
		std::make_shared<io_data_accept>(this, std::move(_sHandler), _sData).get()->shared_from_this()
	).first;
}
	/// create an read request
std::shared_ptr<io_data> io_uring_queue_init::createRead(
	io_data::HANDLER _sHandler,
	const std::shared_ptr<io_data_created_fd> &_rFD,
	const std::shared_ptr<io_data_created_buffer> &_rBuffer
)
{	return *m_sIoData.insert(
		std::make_shared<io_data_read>(this, std::move(_sHandler), _rFD, _rBuffer).get()->shared_from_this()
	).first;
}
	/// create an read request
std::shared_ptr<io_data> io_uring_queue_init::createRecv(
	io_data::HANDLER _sHandler,
	const std::shared_ptr<io_data_created_fd> &_rFD,
	const std::shared_ptr<io_data_created_buffer> &_rBuffer
)
{	return *m_sIoData.insert(
		std::make_shared<io_data_recv>(this, std::move(_sHandler), _rFD, _rBuffer).get()->shared_from_this()
	).first;
}
std::shared_ptr<io_data> io_uring_queue_init::createWrite(
	io_data::HANDLER _sHandler,
	const std::shared_ptr<io_data_created_fd> &_rFD,
	const std::shared_ptr<io_data_created_buffer>&_rData
)
{	return *m_sIoData.insert(
		std::make_shared<io_data_write>(this, std::move(_sHandler), _rFD, _rData).get()->shared_from_this()
	).first;
}
}
}
