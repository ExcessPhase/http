#include "io_data.h"
#include "io_uring_queue_init.h"


namespace foelsche
{
namespace linux
{
void io_data::handleW(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE)
{	m_sHandler(_pRing, _pCQE, getResource(_pRing, _pCQE));
	_pRing->m_sIoData.erase(shared_from_this());
}
}
}
