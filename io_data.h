#pragma once
#include <memory>
#include <functional>
namespace foelsche
{
namespace linux
{
struct io_uring_queue_init;
struct io_data:std::enable_shared_from_this<io_data>
{	typedef std::function<void(io_uring_queue_init*const ring, ::io_uring_cqe* const cqe)> HANDLER;
	HANDLER m_sHandler;
	io_data(HANDLER _s)
		:m_sHandler(std::move(_s))
	{
	}
	virtual ~io_data(void) = default;
	void handleW(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE);
	//virtual void handle(io_uring_queue_init*const, ::io_uring_cqe* const ) = 0;
};
}
}
