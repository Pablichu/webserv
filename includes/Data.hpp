#pragma once

#include <iostream>

#include "Config.hpp"

struct  ServerConfig;
struct  LocationConfig;

// One ConnectionData per client (connection) socket

struct	ConnectionData
{
	std::vector<ServerConfig const *> *	portConfigs;
	std::size_t													serverIndex;
	std::size_t													locationIndex;
	Request															req;
	std::string													ip;
	std::map<std::string, std::string>	urlData;
	int																	fileFd;
	std::string													filePath;
	long																fileSize;
	std::size_t													totalBytesRead;
	std::size_t													totalBytesSent;
	std::size_t													rspSize;
	std::size_t													rspBuffSize;
	std::size_t													rspBuffOffset;
	std::string													rspBuff;

	static std::size_t const						rspBuffCapacity = 8192;

	ConnectionData(void);
	~ConnectionData(void);

	ServerConfig const *		getServer(void);
	LocationConfig					getLocation(void);
};

struct	CgiData
{
	int         socket;
  std::size_t connIndex;
  int         inPipe[2];
  int         outPipe[2];

	CgiData(int const socket, std::size_t const connIndex);

	int getRInPipe(void) const;
  int getWInPipe(void) const;
  int getROutPipe(void) const;
  int getWOutPipe(void) const;
};
