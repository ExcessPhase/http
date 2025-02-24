#include "socket.h"
#include "io_uring_queue_init.h"
#include "io_uring_wait_cqe.h"
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <fcntl.h>
#include <cstring>
#include <fstream>
#include <regex>
#include <string>
#include <filesystem>
namespace foelsche
{
namespace http
{
/*	accept();
	while (true)
	{	bEOF = false;
		do
			if (bEOF = receive())
				break;
		while (no_empty_line_terminator);
		if (bEOF)
			break;
		do
			send(prefix);
		while (not_fully_send);
		do
			read();
			do
				send(file);
			while (not_fully_send);
		while (not_read_until_end_of_file);
	}
*/
using namespace foelsche::linux;
static constexpr const std::size_t BUFFER_SIZE = 16384;
	/// the event loop
	/// blocking
	/// only serves maximally 8 events
static void event_loop(io_uring_queue_init* const ring)
{	while (true)
	{	for (const auto &r : ring->m_sIoData)
			std::cerr << typeid(*r).name() << ":" << r->getFD() << ":" << r->m_iId << std::endl;
		io_uring_wait_cqe sCQE(&ring->m_sRing);
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
	sRing.createAccept(std::make_shared<io_data_created_fd>(sSocket.m_i, false));
		/// call the event loop
	event_loop(&sRing);
}
