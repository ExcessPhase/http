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
	virtual std::shared_ptr<io_data_created> getResource(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE)
	{	return std::make_shared<io_data_created_fd>(_pCQE->res);
	}
	virtual enumType getType(void) const
	{	return eAccept;
	}
};
	/// a read request
struct io_data_recv:io_data
{	std::vector<char> m_sBuffer;
	static constexpr const std::size_t SIZE = 1024;
	io_data_recv(io_uring_queue_init *const _pRing, HANDLER _sHandler, const std::shared_ptr<io_data_created_fd> &_sData)
		:io_data(std::move(_sHandler), _sData),
		m_sBuffer(SIZE)
	{	const auto sFD = std::dynamic_pointer_cast<io_data_created_fd>(m_sData);
		fcntl(sFD->m_iID, F_SETFL, fcntl(sFD->m_iID, F_GETFL, 0) | O_NONBLOCK);
		io_uring_sqe* const sqe = io_uring_queue_init::io_uring_get_sqe(&_pRing->m_sRing);
		sqe->user_data = reinterpret_cast<uintptr_t>(this);
		io_uring_prep_recv(sqe, sFD->m_iID, m_sBuffer.data(), m_sBuffer.size(), 0);
		io_uring_sqe_set_data(sqe, this);
		io_uring_submit(&_pRing->m_sRing);
	}
	virtual std::shared_ptr<io_data_created> getResource(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE)
	{	return std::make_shared<io_data_created_buffer>(std::move(m_sBuffer));
	}
	virtual enumType getType(void) const
	{	return eReceive;
	}
};
}
	/// create an accept request
std::shared_ptr<io_data> io_uring_queue_init::createAccept(io_data::HANDLER _sHandler, const std::shared_ptr<io_data_created_fd> &_sData)
{	return *m_sIoData.insert(
		std::make_shared<io_data_accept>(this, std::move(_sHandler), _sData).get()->shared_from_this()
	).first;
}
	/// create an read request
std::shared_ptr<io_data> io_uring_queue_init::createRecv(io_data::HANDLER _sHandler, const std::shared_ptr<io_data_created_fd> &_sData)
{	return *m_sIoData.insert(
		std::make_shared<io_data_recv>(this, std::move(_sHandler), _sData).get()->shared_from_this()
	).first;
}
int io_uring_queue_init::getServerSocket(const io_data&_r)
{	return std::dynamic_pointer_cast<const io_data_created_fd>(_r.m_sData)->m_iID;
}
int io_uring_queue_init::getFD(io_data_created&_r)
{	return dynamic_cast<const io_data_created_fd&>(_r).m_iID;
}
}
}
