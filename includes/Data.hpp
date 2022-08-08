#pragma once

#include <iostream>

#include "Config.hpp"
#include "Request.hpp"

struct  ServerConfig;
struct  LocationConfig;

struct	CgiData
{
	std::size_t				index;
	int const        	socket;
  std::size_t const	connIndex;
	std::string	const	filePath;
  int         			inPipe[2];
  int         			outPipe[2];

	CgiData(int const socket, std::size_t const connIndex,
					std::string const & filePath);

	int getRInPipe(void) const;
  int getWInPipe(void) const;
  int getROutPipe(void) const;
  int getWOutPipe(void) const;
};

struct FileData
{
	int								fd;
	std::size_t				index;
	int const					socket;
	std::size_t	const	connIndex;
	std::string				filePath;
	long							fileSize;

	FileData(std::string const & filePath, int const socket,
						std::size_t const connIndex);
};

// One ConnectionData per client (connection) socket

struct	ConnectionData
{
	std::vector<ServerConfig const *> *	portConfigs;
	std::size_t													serverIndex;
	std::size_t													locationIndex;
	Request															req;
	std::string													ip;
	std::map<std::string, std::string>	urlData;
	std::size_t													totalBytesRead;
	std::size_t													totalBytesSent;
	std::size_t													rspSize;
	std::size_t													rspBuffSize;
	std::size_t													rspBuffOffset;
	std::string													rspBuff;
	int																	rspStatus;
	FileData *													fileData;
	CgiData *														cgiData;

	static std::size_t const						rspBuffCapacity = 8192;

	ConnectionData(void);
	~ConnectionData(void);

	ServerConfig const *		getServer(void);
	LocationConfig const *	getLocation(void);
};
