#include "io_uring_queue_init.h"


namespace foelsche
{
namespace linux
{
namespace
{
struct io_data_accept:io_data
{	struct sockaddr_in client_addr;
	socklen_t client_len;
	const int m_iServerSocket;
	//foelsche::linux::io_uring_queue_init *const ring;

	io_data_accept(io_uring_queue_init *const _pRing, const int _iFD, HANDLER _sHandler)
		:io_data(std::move(_sHandler)),
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
};
struct io_data_recv:io_data
{	io_data_recv(io_uring_queue_init *const _pRing, const int _iFD, HANDLER _sHandler)
		:io_data(std::move(_sHandler))
	{
	}
};
}
std::shared_ptr<io_data> io_uring_queue_init::createAccept(const int _i)
{	return *m_sIoData.insert(std::make_shared<io_data_accept>(this, _i).get()->shared_from_this()).first;
}
std::shared_ptr<io_data> io_uring_queue_init::createRecv(const int _i)
{
}
}
}
