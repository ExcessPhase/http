#pragma once
#include <liburing.h>
#include <memory>
#include <functional>
namespace foelsche
{
namespace linux
{
struct io_uring_queue_init;
struct io_data_created:std::enable_shared_from_this<io_data_created>
{	virtual ~io_data_created(void) = default;
};
struct io_data:std::enable_shared_from_this<io_data>
{	typedef std::function<void(io_uring_queue_init*const ring, ::io_uring_cqe* const cqe, std::shared_ptr<io_data_created>) > HANDLER;
	HANDLER m_sHandler;
	std::shared_ptr<io_data_created> m_sData;
	io_data(HANDLER _s, std::shared_ptr<io_data_created> _sData)
		:m_sHandler(std::move(_s)),
		m_sData(std::move(_sData))
	{
	}
	virtual ~io_data(void) = default;
	virtual std::shared_ptr<io_data_created> getResource(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE) = 0;
	void handleW(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE);
	enum enumType
	{	eAccept,
		eReceive
	};
	virtual enumType getType(void) const = 0;
	//virtual void handle(io_uring_queue_init*const, ::io_uring_cqe* const ) = 0;
};
}
}
