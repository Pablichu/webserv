#pragma once

#include <iostream>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <dirent.h>

#include "Data.hpp"

struct	ConnectionData;

struct	InitStatusCode
{
	std::map<int const, std::string const>	m;

	InitStatusCode(void);
};

struct	InitContentType
{
	std::map<std::string const, std::string const>	m;

	InitContentType(void);
};

class	Response
{
private:

	std::size_t	bytesSent;

	void	_buildResponse(ConnectionData & connData, std::string const & content);

public:

	static std::string const									protocol; // HTTP/1.1
	static std::map<int const, std::string const> const			statusCode;
	static std::map<std::string const, std::string const> const	contentType;

	Response(void);
	~Response(void);

	void	buildRedirect(ConnectionData & connData, std::string const & url);
	void	buildDirList(ConnectionData & connData, std::string const & uri,
											std::string const & root);
	void	buildError(ConnectionData & connData, int const error);
	bool	sendData(int const sockFd, ConnectionData & connData);
};
