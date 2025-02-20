#include "socket.h"
#include "epoll_create1.h"
#include "io_uring_queue_init.h"
#include "io_uring_wait_cqe.h"
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <fcntl.h>
#include <cstring>
#include <map>
namespace foelsche
{
namespace http
{
using namespace foelsche::linux;
	/// handler for read() system call
static const io_data::HANDLER s_sReceive = [](io_uring_queue_init*const ring, ::io_uring_cqe* const cqe, const std::shared_ptr<io_data_created> &_sData, io_data&_r)
{	auto &r = std::dynamic_pointer_cast<io_data_created_buffer>(_sData)->m_s;
		/// shrink the buffer to match the received number of bytes
	r.resize(cqe->res);
	if (!r.empty())
	{		/// create a new async read()
		ring->createRecv(s_sReceive, std::dynamic_pointer_cast<io_data_created_fd>(_r.m_sData));
			/// printout the data received
		for (const auto c : r)
			if (std::isprint(c))
				std::cerr << c;
			else
				std::cerr << "\\0x" << unsigned(c);
		std::cerr << std::endl;
	}
};
	/// handler for accept() system call
static const io_data::HANDLER s_sAccept = [](io_uring_queue_init*const ring, ::io_uring_cqe* const cqe, const std::shared_ptr<io_data_created> &_sData, io_data&_r)
{		/// create a new async IO form accept
	ring->createAccept(s_sAccept, std::dynamic_pointer_cast<io_data_created_fd>(_r.m_sData));
		/// create an async read()
	ring->createRecv(s_sReceive, std::dynamic_pointer_cast<io_data_created_fd>(_sData));
};
	/// map of enum to handler
static const std::map<io_data::enumType, io_data::HANDLER> sType2Handler = {
	{	io_data::eAccept,
		s_sAccept
	},
	{	io_data::eReceive,
		s_sReceive
	}
};
	/// the event loop
	/// blocking
	/// only serves maximally 8 events
static void event_loop(io_uring_queue_init* const ring)
{	for (std::size_t i = 0; i < 8; ++i)
	{	io_uring_wait_cqe sCQE(&ring->m_sRing);
		if (sCQE.m_pCQE->res < 0)
			std::cerr << "Async operation failed: " << std::strerror(-sCQE.m_pCQE->res) << std::endl;
		else
		{	auto data(static_cast<io_data*>(io_uring_cqe_get_data(sCQE.m_pCQE))->shared_from_this());
			data->handleW(ring, sCQE.m_pCQE);
		}
	}
}
}
}
int main(int, char**)
{	using namespace foelsche::linux;
	using namespace foelsche::http;

	constexpr int QUEUE_DEPTH = 256;
	constexpr int PORT = 8080;
		/// RAII initialize and destroy a uring structre
	foelsche::linux::io_uring_queue_init sRing(QUEUE_DEPTH, 0);
		/// RAII initialize and destroy a socket
	foelsche::linux::socket sSocket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
		/// some property-setting
		/// which can result in an exception
	{	struct sockaddr_in server_addr;
		std::fill(
			reinterpret_cast<char*>(&server_addr),
			reinterpret_cast<char*>(1 + &server_addr),
			0
		);
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = INADDR_ANY;
		server_addr.sin_port = htons(PORT);
		socket::bind(sSocket.m_i, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof server_addr);
		socket::listen(sSocket.m_i, SOMAXCONN);
	}
		/// create an accept object waiting for an incoming connection request
	sRing.createAccept(s_sAccept, std::make_shared<io_data_created_fd>(sSocket.m_i, false));
		/// call the event loop
	event_loop(&sRing);
}
