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
enum class command
{	ACCEPT,
	READ,
	WRITE
};
static const io_data::HANDLER s_sReceive = [](io_uring_queue_init*const ring, ::io_uring_cqe* const cqe, const std::shared_ptr<io_data_created> &_sData, io_data&_r)
{
	auto &r = std::dynamic_pointer_cast<io_data_created_buffer>(_sData)->m_s;
	r.resize(cqe->res);
	if (!r.empty())
	{	ring->createRecv(s_sReceive, std::dynamic_pointer_cast<io_data_created_fd>(_r.m_sData));
		for (const auto c : r)
			if (std::isprint(c))
				std::cerr << c;
		std::cerr << std::endl;
	}
};
static const io_data::HANDLER s_sAccept = [](io_uring_queue_init*const ring, ::io_uring_cqe* const cqe, const std::shared_ptr<io_data_created> &_sData, io_data&_r)
{	ring->createAccept(s_sAccept, std::dynamic_pointer_cast<io_data_created_fd>(_r.m_sData));
	ring->createRecv(s_sReceive, std::dynamic_pointer_cast<io_data_created_fd>(_sData));
};
static void handle_accept(io_uring_queue_init* ring, io_uring_cqe* cqe, std::unique_ptr<io_data> data, const int server_socket_fd);
static const std::map<io_data::enumType, io_data::HANDLER> sType2Handler = {
	{	io_data::eAccept,
		s_sAccept
	},
	{	io_data::eReceive,
		s_sReceive
	}
};
static void event_loop(io_uring_queue_init* const ring)
{	for (std::size_t i = 0; i < 8; ++i)
	{	//io_uring_cqe* cqe;
		//int ret = io_uring_wait_cqe(ring, &cqe);
		io_uring_wait_cqe sCQE(&ring->m_sRing);

		// Reclaim ownership of the io_data pointer

		if (sCQE.m_pCQE->res < 0)
		{	std::cerr << "Async operation failed: " << std::strerror(-sCQE.m_pCQE->res) << std::endl;
			//close(data->fd);
		}
		else
		{	auto data(static_cast<io_data*>(io_uring_cqe_get_data(sCQE.m_pCQE))->shared_from_this());
			data->handleW(ring, sCQE.m_pCQE);//, sType2Handler.at(data->getType()));
#if 0
			switch (command(data->operation))
			{	case command::ACCEPT:
					handle_accept(ring, cqe, std::move(data), server_socket_fd);
					break;
				default:
					std::cerr << "Unknown operation" << std::endl;
					//close(data->fd);
					break;
			}
#endif
		}
	}
}
static void submit_accept(foelsche::linux::io_uring_queue_init *const ring, const int);
static void handle_accept(foelsche::linux::io_uring_queue_init* const ring, io_uring_cqe* const cqe, std::unique_ptr<io_data> data, const int server_socket_fd)
{
#if 0
	int client_fd = cqe->res;
	std::cout << "Accepted connection. Socket FD: " << client_fd << std::endl;

	int flags = fcntl(client_fd, F_GETFL, 0);
	fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

	data->fd = client_fd;
	data->operation = int(command::READ);

	io_uring_sqe* const sqe = io_uring_get_sqe(ring);
	if (!sqe)
	{	std::cerr << "Could not get SQE for client read" << std::endl;
		close(client_fd);
		return;
	}

	io_uring_prep_recv(sqe, client_fd, data->buffer, sizeof(data->buffer), 0);

	// Release the unique_ptr to avoid double-deletion
	io_uring_sqe_set_data(sqe, data.release());

	io_uring_submit(ring);

	// Submit another accept request to keep accepting new connections
	submit_accept(ring, server_socket_fd);
#endif
}
static void submit_accept(foelsche::linux::io_uring_queue_init *const ring, const int server_socket_fd)
{
#if 0
	io_uring_sqe* const sqe = io_uring_get_sqe(ring);
	 // Use std::unique_ptr for automatic memory management
	std::unique_ptr<io_data> data = std::make_unique<io_data>();
	data->fd = server_socket_fd;
	data->operation = int(command::ACCEPT);
	data->client_len = sizeof(struct sockaddr_in);
	io_uring_prep_accept(
		sqe,
		server_socket_fd,
		reinterpret_cast<struct sockaddr*>(&data->client_addr),
		&data->client_len,
		0
	);
	// Release the unique_ptr to avoid double-deletion
	io_uring_sqe_set_data(sqe, data.release());
	io_uring_submit(ring);
#endif
}
}
}
int main(int, char**)
{	using namespace foelsche::linux;
	using namespace foelsche::http;

	constexpr int QUEUE_DEPTH = 256;
	constexpr int PORT = 8080;
	foelsche::linux::io_uring_queue_init sRing(QUEUE_DEPTH, 0);
	foelsche::linux::socket sSocket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
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
	sRing.createAccept(s_sAccept, std::make_shared<io_data_created_fd>(sSocket.m_i, false));
	event_loop(&sRing);
}
