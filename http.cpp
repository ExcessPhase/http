#include "socket.h"
#include "io_uring_queue_init.h"
#include "io_uring_wait_cqe.h"
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <fcntl.h>
#include <cstring>
#include <map>
#include <fstream>
#include <regex>
#include <string>
#include <filesystem>
namespace foelsche
{
namespace http
{
using namespace foelsche::linux;
static constexpr const std::size_t BUFFER_SIZE = 16384;
static const io_data::HANDLER &getReceive(void);
static const io_data::HANDLER s_sWrite = [](io_uring_queue_init*const ring, ::io_uring_cqe* const cqe, const std::shared_ptr<io_data_created> &_sData, io_data&_r)
{	const auto sBuffer = io_data::getWriteBuffer(_r);
	const auto iOffset = io_data::getWriteOffset(_r);
	std::cerr << "write request finished " << cqe->res << std::endl;
	if (iOffset + cqe->res < sBuffer->m_s.size())
	{	std::cerr << "scheduling missing write for missing data of " << sBuffer->m_s.size() - iOffset + cqe->res << std::endl;
		sBuffer->m_iOffset += cqe->res;
		ring->createWrite(
			s_sWrite,
			std::dynamic_pointer_cast<io_data_created_fd>(_r.m_sData),
			sBuffer
		);
	}
	else
	{	std::cerr << "scheduling read" << std::endl;
		ring->createRecv(getReceive(), std::dynamic_pointer_cast<io_data_created_fd>(_r.m_sData), std::make_shared<io_data_created_buffer>(std::vector<char>(BUFFER_SIZE), 0));
	}
};
static std::string getPrefix(const char *const _pFileName)
{	const std::filesystem::path sPath(_pFileName);
	if (sPath.extension() == ".png")
		return "HTTP/1.1 200 OK\r\n"
			"Content-Type: image/png\r\n"
			"Content-Length: " + std::to_string(file_size(sPath)) + "\r\n"
			//"Connection: close\r\n"
			"\r\n";
	else
	if (sPath.extension() == ".css")
		return "HTTP/1.1 200 OK\r\n"
			"Content-Type: text/css\r\n"
			"Content-Length: " + std::to_string(file_size(sPath)) + "\r\n"
			"\r\n";
	else
		return "HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: " + std::to_string(file_size(sPath)) + "\r\n"
			"\r\n";
}
static std::vector<char> read_file_to_vector(const char*const filename)
{
	std::ifstream file(filename, std::ios::binary);

	std::cerr << "opening file: " << filename << std::endl;
	// Check if the file is open
	if (!file.is_open()) {
		static const auto p =
"HTTP/1.1 404 Not Found\r\n"
"Content-Type: text/html\r\n"
"Content-Length: 142\r\n"
"\r\n"
"<!DOCTYPE html>\r\n"
"<html>\r\n"
"<head>\r\n"
"    <title>404 Not Found</title>\r\n"
"</head>\r\n"
"<body>\r\n"
"    <h1>404 Not Found</h1>\r\n"
"    <p>The requested resource could not be found on this server.</p>\r\n"
"</body>\r\n"
"</html>\r\n";
		return std::vector<char>(p, p + std::strlen(p));
	}

	// Read file contents into the vector
	const auto sPrefix = getPrefix(filename);
	std::cerr << "prefix: " << sPrefix << std::endl;
	std::vector<char> s(sPrefix.begin(), sPrefix.end());
	s.insert(
		s.end(),
		std::istreambuf_iterator<char>(file),
		std::istreambuf_iterator<char>()
	);
	std::cerr << "data size = " << s.size() << std::endl;
	std::cerr << "sPrefix size = " << sPrefix.size() << std::endl;
#if 0
	for (const auto c : s)
		if (std::isprint(c))
			std::cerr << c;
		else
			std::cerr << "\\0x" << int(c);
	std::cerr << std::endl;
#endif
	return s;
}
static std::string parse_filename(const std::string& request)
{	static const std::regex get_regex(R"(GET\s\/([^?\s]+)\sHTTP\/1\.1)");
	std::smatch match;

	if (std::regex_search(request, match, get_regex) && match.size() > 1)
		return match.str(1);
	else
		return "index.html";
}
	/// handler for read() system call
static const io_data::HANDLER s_sReceive = [](io_uring_queue_init*const ring, ::io_uring_cqe* const cqe, const std::shared_ptr<io_data_created> &_sData, io_data&_r)
{
	if (cqe->res > 0)
	{	auto pBuffer = std::dynamic_pointer_cast<io_data_created_buffer>(_sData);
		auto &iOffset = pBuffer->m_iOffset;
		auto &rBuffer = pBuffer->m_s;
		rBuffer.resize(iOffset + cqe->res);
		if (rBuffer.size() > 3 && std::equal(
			rBuffer.cbegin() + rBuffer.size() - 4,
			rBuffer.cbegin() + rBuffer.size(),
			"\r\n\r\n"
		))
		{
#if 1
			for (const auto c : rBuffer)
				if (std::isprint(c))
					std::cerr << c;
				else
					std::cerr << "\\0x" << unsigned(c);
			std::cerr << std::endl;
#endif
				/// printout the data received
			ring->createWrite(
				s_sWrite,
				std::dynamic_pointer_cast<io_data_created_fd>(_r.m_sData),
				std::make_shared<io_data_created_buffer>(
					read_file_to_vector(
						parse_filename(std::string(rBuffer.begin(), rBuffer.end())).c_str()
					),
					0
				)
			);
		}
		else
		{	iOffset += cqe->res;
			rBuffer.resize(iOffset + BUFFER_SIZE);
			ring->createRecv(s_sReceive, std::dynamic_pointer_cast<io_data_created_fd>(_r.m_sData), pBuffer);
		}
	}
	else
		if (cqe->res == 0)
			std::cerr << "EOF=" << _r.getFD() << std::endl;
		else
			std::cerr << "error=" << std::strerror(-cqe->res) << ":" << _r.getFD() << std::endl;
};
static const io_data::HANDLER &getReceive(void)
{	return s_sReceive;
}
	/// handler for accept() system call
static const io_data::HANDLER s_sAccept = [](io_uring_queue_init*const ring, ::io_uring_cqe* const cqe, const std::shared_ptr<io_data_created> &_sData, io_data&_r)
{		/// create a new async IO form accept
	ring->createAccept(s_sAccept, std::dynamic_pointer_cast<io_data_created_fd>(_r.m_sData));
		/// create an async read()
	ring->createRecv(s_sReceive, std::dynamic_pointer_cast<io_data_created_fd>(_sData), std::make_shared<io_data_created_buffer>(std::vector<char>(BUFFER_SIZE), 0));
};
	/// map of enum to handler
static const std::map<io_data::enumType, io_data::HANDLER> sType2Handler = {
	{	io_data::eAccept,
		s_sAccept
	},
	{	io_data::eReceive,
		s_sReceive
	},
	{	io_data::eWrite,
		s_sWrite
	}
};
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
	sRing.createAccept(s_sAccept, std::make_shared<io_data_created_fd>(sSocket.m_i, false));
		/// call the event loop
	event_loop(&sRing);
}
