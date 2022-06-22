#pragma once

#include <iostream>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

#include "Data.hpp"

struct	ConnectionData;

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

	std::size_t								bytesRead;
	std::size_t								bytesSent;

public:
	Response();
	~Response();

	bool	openFile(std::string const & filePath, int & fd);
	bool	readFileFirst(int const fd, ConnectionData & connData);
	bool	readFileNext(int const fd, ConnectionData & connData);
	bool	sendFile(int const sockFd, ConnectionData & connData);
};
