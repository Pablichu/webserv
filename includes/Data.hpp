#pragma once

#include <iostream>
#include <poll.h>
#include <time.h>

#include "Config.hpp"
#include "Request.hpp"
#include "InputOutput.hpp"

struct  ServerConfig;
struct  LocationConfig;

struct	CgiData
{
	pollfd &					socket;
	std::string const interpreterPath;
	std::string	const	scriptPath;
  int         			inPipe[2];
  int         			outPipe[2];
	pid_t							pID;

	CgiData(pollfd & socket, std::string const & interpreterPath,
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
	pollfd &		socket;
	std::string	filePath;
	long				fileSize;
	FileOp			fileOp;

	FileData(std::string const & filePath, pollfd & socket);
};

enum	ConnectionStatus
{
	Active,
	Idle
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
	InputOutput													io;
	int																	rspStatus;
	ConnectionStatus										status;
	time_t															lastActive;
	int																	handledRequests;
	FileData *													fileData;
	CgiData *														cgiData;

	// Maximum time (seconds) a connection can stay idle.
	static double const									timeout;
	// Maximum requests that can reuse each connection.
	static int const										max;

	ConnectionData(void);
	~ConnectionData(void);

	ServerConfig const *		getServer(void);
	LocationConfig const *	getLocation(void);
	void										setIdle(void);
};
