#pragma once

#include <iostream>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

class	Response
{
private:
	//Status line
	static std::string const	protocol; // HTTP/1.1
	int												status;
	std::string								status_msg;

	//Headers
	//std::string	cnt_encoding;
	std::string	cnt_type; //	Usually text/html; charset=utf-8
	std::size_t	cnt_length;

	static std::size_t const	buffSize = 8192;

	char											buff[Response::buffSize + 1]; // Povisional
	std::size_t								bytesRead;
	std::size_t								bytesSent;

public:
	Response();
	~Response();

	bool	openFile(std::string const & filePath, int & fd);
	bool	readFile(std::string const & filePath, int const fd,
									std::string & rsp, std::size_t & totalBytesRead,
									long & fileSize); //Too many args
	bool	readFile(int const fd, std::string & rsp, std::size_t & totalBytesRead);
	bool	sendFile(int const sockFd, std::string & rsp,
									std::size_t & totalBytesSent);
};
