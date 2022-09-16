#pragma once

#include <iostream>

#include "Config.hpp"
#include "Request.hpp"

struct  ServerConfig;
struct  LocationConfig;

struct	CgiData
{
	int const        	socket;
	std::string const interpreterPath;
	std::string	const	scriptPath;
  int         			inPipe[2];
  int         			outPipe[2];
	pid_t							pID;

	CgiData(int const socket, std::string const & interpreterPath,
					std::string const & scriptPath);

	int getRInPipe(void) const;
  int getWInPipe(void) const;
  int getROutPipe(void) const;
  int getWOutPipe(void) const;
};

enum	FileOp
{
	Read,
	Create,
	Append
};

struct FileData
{
	int					fd;
	int const		socket;
	std::string	filePath;
	long				fileSize;
	FileOp			fileOp;

	FileData(std::string const & filePath, int const socket);
};

// One ConnectionData per client (connection) socket

struct	ConnectionData
{
	std::vector<ServerConfig const *>		* portConfigs;
	std::size_t								serverIndex;
	std::size_t								locationIndex;
	std::string								ip;
	std::map<std::string, std::string>		urlData;
	std::size_t								totalBytesRead;
	std::size_t								totalBytesSent;
	std::size_t								rspSize;
	std::size_t								buffSize;
	std::size_t								buffOffset;
	std::string								buff;
	int										rspStatus;
	FileData *								fileData;
	CgiData *								cgiData;
	Request									req;

	static std::size_t const						buffCapacity = 8192;

	ConnectionData(void);
	~ConnectionData(void);

	ServerConfig const *		getServer(void);
	LocationConfig const *	getLocation(void);
};
