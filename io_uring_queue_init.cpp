#include "io_uring_queue_init.h"
#include <netinet/in.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <cstring>
#include <filesystem>


namespace foelsche
{
namespace linux
{
namespace
{
	/// an accept request
struct io_data_accept:io_data
{	struct sockaddr_in client_addr;
	socklen_t client_len;
	io_data_accept(io_uring_queue_init *const _pRing, const std::shared_ptr<io_data_created_fd> &_sData)
		:io_data(_sData),
		client_len(sizeof(struct sockaddr_in))
	{	io_uring_sqe* const sqe = io_uring_queue_init::io_uring_get_sqe(&_pRing->m_sRing);
		sqe->user_data = reinterpret_cast<uintptr_t>(this);
		io_uring_prep_accept(
			sqe,
			std::dynamic_pointer_cast<io_data_created_fd>(m_sData)->m_iID,
			reinterpret_cast<struct sockaddr*>(&client_addr),
			&client_len,
			0
		);
		io_uring_sqe_set_data(sqe, this);
		io_uring_submit(&_pRing->m_sRing);
	}
	virtual std::shared_ptr<io_data_created> getResource(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE) override
	{	return std::make_shared<io_data_created_fd>(_pCQE->res);
	}
	virtual int getFD(void) const override
	{	return std::dynamic_pointer_cast<io_data_created_fd>(m_sData)->m_iID;
	}
	virtual void handle(io_uring_queue_init*const ring, ::io_uring_cqe* const cqe) override
	{		/// create a new async IO form accept
		ring->createAccept(std::dynamic_pointer_cast<io_data_created_fd>(m_sData));
			/// create an async read()
		ring->createRecv(std::dynamic_pointer_cast<io_data_created_fd>(getResource(ring, cqe)), std::make_shared<io_data_created_buffer>(std::vector<char>(BUFFER_SIZE), 0));
	}
};
	/// a read request (from file)
#if 0
struct io_data_read:io_data
{	const std::shared_ptr<io_data_created_buffer> m_sBuffer;
	io_data_read(
		io_uring_queue_init *const _pRing,
		const std::shared_ptr<io_data_created_fd> &_rFD,
		const std::shared_ptr<io_data_created_buffer> &_rBuffer
	)
		:io_data(_rFD),
		m_sBuffer(_rBuffer)
	{	const auto sFD = std::dynamic_pointer_cast<io_data_created_fd>(m_sData);
		fcntl(sFD->m_iID, F_SETFL, fcntl(sFD->m_iID, F_GETFL, 0) | O_NONBLOCK);
		io_uring_sqe* const sqe = io_uring_queue_init::io_uring_get_sqe(&_pRing->m_sRing);
		sqe->user_data = reinterpret_cast<uintptr_t>(this);
		io_uring_prep_read(sqe, sFD->m_iID, m_sBuffer->m_s.data() + m_sBuffer->m_iOffset, m_sBuffer->m_s.size() - m_sBuffer->m_iOffset, 0);
		io_uring_sqe_set_data(sqe, this);
		io_uring_submit(&_pRing->m_sRing);
	}
	virtual std::shared_ptr<io_data_created> getResource(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE) override
	{	return m_sBuffer;
	}
	virtual int getFD(void) const override
	{	return std::dynamic_pointer_cast<io_data_created_fd>(m_sData)->m_iID;
	}
	virtual void handle(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE) override;
};
#endif
static std::string parse_filename(const std::string& request)
{	static const std::regex get_regex(R"(GET\s\/([^?\s]+)\sHTTP\/1\.1)");
	std::smatch match;

	if (std::regex_search(request, match, get_regex) && match.size() > 1)
		return match.str(1);
	else
		return "index.html";
}
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
	/// a receive request (from socket)
struct io_data_recv:io_data
{	const std::shared_ptr<io_data_created_buffer> m_sBuffer;
	io_data_recv(
		io_uring_queue_init *const _pRing,
		const std::shared_ptr<io_data_created_fd> &_rFD,
		const std::shared_ptr<io_data_created_buffer> &_rBuffer
	)
		:io_data(_rFD),
		m_sBuffer(_rBuffer)
	{	const auto sFD = std::dynamic_pointer_cast<io_data_created_fd>(m_sData);
		fcntl(sFD->m_iID, F_SETFL, fcntl(sFD->m_iID, F_GETFL, 0) | O_NONBLOCK);
		io_uring_sqe* const sqe = io_uring_queue_init::io_uring_get_sqe(&_pRing->m_sRing);
		sqe->user_data = reinterpret_cast<uintptr_t>(this);
		io_uring_prep_recv(sqe, sFD->m_iID, m_sBuffer->m_s.data() + m_sBuffer->m_iOffset, m_sBuffer->m_s.size() - m_sBuffer->m_iOffset, 0);
		io_uring_sqe_set_data(sqe, this);
		io_uring_submit(&_pRing->m_sRing);
	}
	virtual std::shared_ptr<io_data_created> getResource(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE) override
	{	return m_sBuffer;
	}
	virtual int getFD(void) const override
	{	return std::dynamic_pointer_cast<io_data_created_fd>(m_sData)->m_iID;
	}
	virtual void handle(io_uring_queue_init*const ring, ::io_uring_cqe* const cqe) override
	{	if (cqe->res > 0)
		{	//auto pBuffer = std::dynamic_pointer_cast<io_data_created_buffer>(_sData);
			auto &iOffset = m_sBuffer->m_iOffset;
			auto &rBuffer = m_sBuffer->m_s;
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
				ring->createSend(
					std::dynamic_pointer_cast<io_data_created_fd>(m_sData),
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
				ring->createRecv(std::dynamic_pointer_cast<io_data_created_fd>(m_sData), m_sBuffer);
			}
		}
		else
			if (cqe->res == 0)
				std::cerr << "EOF=" << getFD() << std::endl;
			else
				std::cerr << "error=" << std::strerror(-cqe->res) << ":" << getFD() << std::endl;
	}
};
struct io_data_send:io_data
{	const std::shared_ptr<io_data_created_buffer> m_sBuffer;
	io_data_send(
		io_uring_queue_init *const _pRing,
		const std::shared_ptr<io_data_created_fd> &_rFD,
		const std::shared_ptr<io_data_created_buffer>&_rData
	)
		:io_data(
			_rFD
		),
		m_sBuffer(_rData)
	{
		io_uring_sqe* const sqe = io_uring_queue_init::io_uring_get_sqe(&_pRing->m_sRing);
		sqe->user_data = reinterpret_cast<uintptr_t>(this);
		io_uring_prep_send(
			sqe,
			_rFD->m_iID,
			_rData->m_s.data() + _rData->m_iOffset,
			_rData->m_s.size() - _rData->m_iOffset,
			0
		);
		io_uring_sqe_set_data(sqe, this);
		io_uring_submit(&_pRing->m_sRing);
	}
	virtual std::shared_ptr<io_data_created> getResource(io_uring_queue_init*const _pRing, ::io_uring_cqe* const _pCQE) override
	{	return nullptr;
	}
	virtual int getFD(void) const override
	{	return std::dynamic_pointer_cast<io_data_created_fd>(m_sData)->m_iID;
	}
	virtual void handle(io_uring_queue_init*const ring, ::io_uring_cqe* const cqe) override
	{	const auto iOffset = m_sBuffer->m_iOffset;
		std::cerr << "write request finished " << cqe->res << std::endl;
		if (iOffset + cqe->res < m_sBuffer->m_s.size())
		{	std::cerr << "scheduling missing write for missing data of " << m_sBuffer->m_s.size() - iOffset + cqe->res << std::endl;
			m_sBuffer->m_iOffset += cqe->res;
			ring->createSend(
				std::dynamic_pointer_cast<io_data_created_fd>(m_sData),
				m_sBuffer
			);
		}
		else
		{	std::cerr << "scheduling read" << std::endl;
			ring->createRecv(std::dynamic_pointer_cast<io_data_created_fd>(m_sData), std::make_shared<io_data_created_buffer>(std::vector<char>(BUFFER_SIZE), 0));
		}
	}
};
}
	/// create an accept request
std::shared_ptr<io_data> io_uring_queue_init::createAccept(const std::shared_ptr<io_data_created_fd> &_sData)
{	return *m_sIoData.insert(
		std::make_shared<io_data_accept>(this, _sData).get()->shared_from_this()
	).first;
}
	/// create an read request
#if 0
std::shared_ptr<io_data> io_uring_queue_init::createRead(
	const std::shared_ptr<io_data_created_fd> &_rFD,
	const std::shared_ptr<io_data_created_buffer> &_rBuffer
)
{	return *m_sIoData.insert(
		std::make_shared<io_data_read>(this, _rFD, _rBuffer).get()->shared_from_this()
	).first;
}
#endif
	/// create an read request
std::shared_ptr<io_data> io_uring_queue_init::createRecv(
	const std::shared_ptr<io_data_created_fd> &_rFD,
	const std::shared_ptr<io_data_created_buffer> &_rBuffer
)
{	return *m_sIoData.insert(
		std::make_shared<io_data_recv>(this, _rFD, _rBuffer).get()->shared_from_this()
	).first;
}
std::shared_ptr<io_data> io_uring_queue_init::createSend(
	const std::shared_ptr<io_data_created_fd> &_rFD,
	const std::shared_ptr<io_data_created_buffer>&_rData
)
{	return *m_sIoData.insert(
		std::make_shared<io_data_send>(this,  _rFD, _rData).get()->shared_from_this()
	).first;
}
}
}
