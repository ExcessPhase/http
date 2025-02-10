#include "http.h"
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <fcntl.h>
#include <cstring>
struct io_data
{	int fd;
	int operation;
	struct sockaddr_in client_addr;
	socklen_t client_len;
	char buffer[1024];
};
enum class command
{	ACCEPT,
	READ,
	WRITE
};
static void handle_accept(io_uring* ring, io_uring_cqe* cqe, std::unique_ptr<io_data> data, const int server_socket_fd);
static void event_loop(io_uring* const ring, const int server_socket_fd)
{	while (true)
	{	io_uring_cqe* cqe;
		int ret = io_uring_wait_cqe(ring, &cqe);
		if (ret < 0)
		{	std::cerr << "io_uring_wait_cqe failed: " << std::strerror(-ret) << std::endl;
			break;
		}

		// Reclaim ownership of the io_data pointer
		std::unique_ptr<io_data> data(static_cast<io_data*>(io_uring_cqe_get_data(cqe)));

		if (cqe->res < 0)
		{	std::cerr << "Async operation failed: " << std::strerror(-cqe->res) << std::endl;
			close(data->fd);
		}
		else
			switch (command(data->operation))
			{	case command::ACCEPT:
					handle_accept(ring, cqe, std::move(data), server_socket_fd);
					break;
				default:
					std::cerr << "Unknown operation" << std::endl;
					close(data->fd);
					break;
			}
		// Mark the CQE as seen
		io_uring_cqe_seen(ring, cqe);
	}
}
static void submit_accept(io_uring *const ring, const int);
static void handle_accept(io_uring* const ring, io_uring_cqe* const cqe, std::unique_ptr<io_data> data, const int server_socket_fd)
{
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
}
static void submit_accept(io_uring *const ring, const int server_socket_fd)
{	io_uring_sqe* const sqe = io_uring_get_sqe(ring);
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
}
int main(int, char**)
{	using namespace foelsche::linux;

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
	submit_accept(&sRing.m_sRing, sSocket.m_i);
	event_loop(&sRing.m_sRing, sSocket.m_i);
}
