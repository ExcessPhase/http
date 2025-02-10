#include "io_uring_queue_init.h"
#include <netinet/in.h>


namespace foelsche
{
namespace linux
{
namespace
{
struct io_data_created_fd:io_data_created
{	const int m_iID;
	io_data_created_fd(const int _i)
		:m_iID(_i)
	{
	}
	~io_data_created_fd(void)
	{	::close(m_iID);
	}
};
struct io_data_accept:io_data
{	struct sockaddr_in client_addr;
	socklen_t client_len;
	const int m_iServerSocket;
	//foelsche::linux::io_uring_queue_init *const ring;

	io_data_accept(io_uring_queue_init *const _pRing, const int _iFD, HANDLER _sHandler, std::shared_ptr<io_data_created> _sData)
		:io_data(std::move(_sHandler), std::move(_sData)),
		client_len(sizeof(struct sockaddr_in)),
		m_iServerSocket(_iFD)
		//ring(_pRing)
	{	io_uring_sqe* const sqe = io_uring_queue_init::io_uring_get_sqe(&_pRing->m_sRing);
		io_uring_prep_accept(
			sqe,
			_iFD,
			reinterpret_cast<struct sockaddr*>(&client_addr),
			&client_len,
			0
		);
		// Release the unique_ptr to avoid double-deletion
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
struct io_data_recv:io_data
{	io_data_recv(io_uring_queue_init *const _pRing, const int _iFD, HANDLER _sHandler, std::shared_ptr<io_data_created> _sData)
		:io_data(std::move(_sHandler), std::move(_sData))
	{
	}
	virtual std::shared_ptr<io_data_created> getResource(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE)
	{	return nullptr;
	}
	virtual enumType getType(void) const
	{	return eReceive;
	}
};
}
std::shared_ptr<io_data> io_uring_queue_init::createAccept(const int _i, io_data::HANDLER _sHandler, std::shared_ptr<io_data_created> _sData)
{	return *m_sIoData.insert(
		std::make_shared<io_data_accept>(this, _i, std::move(_sHandler), std::move(_sData)).get()->shared_from_this()
	).first;
}
std::shared_ptr<io_data> io_uring_queue_init::createRecv(const int _i, io_data::HANDLER _sHandler, std::shared_ptr<io_data_created> _sData)
{	return *m_sIoData.insert(
		std::make_shared<io_data_recv>(this, _i, std::move(_sHandler), std::move(_sData)).get()->shared_from_this()
	).first;
}
int io_uring_queue_init::getServerSocket(const io_data&_r)
{	return dynamic_cast<const io_data_accept&>(_r).m_iServerSocket;
}
int io_uring_queue_init::getFD(io_data_created&_r)
{	return dynamic_cast<const io_data_created_fd&>(_r).m_iID;
}
}
}
