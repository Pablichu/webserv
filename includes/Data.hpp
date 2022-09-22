#pragma once

#include <iostream>
#include <poll.h>
#include <time.h>
#include <dirent.h>

#include "Config.hpp"
#include "Request.hpp"
#include "InputOutput.hpp"

struct  ServerConfig;
struct  LocationConfig;

struct	CgiData
{
	/*
	**	If Monitor is reallocated, this pollfd will change,
	**	that is why a pointer is used, not a reference.
	*/
	int const					connFd;
	std::string const interpreterPath;
	std::string	const	scriptPath;
  int         			inPipe[2];
  int         			outPipe[2];
	pid_t							pID;

	CgiData(int const connFd, std::string const & interpreterPath,
					std::string const & scriptPath);

	int		getRInPipe(void) const;
  int		getWInPipe(void) const;
  int		getROutPipe(void) const;
  int		getWOutPipe(void) const;
	void	closeWInPipe(void);
	void	closeROutPipe(void);
	void	closePipes(void);
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
	/*
	**	If Monitor is reallocated, this pollfd will change,
	**	that is why a pointer is used, not a reference.
	*/
	int const		connFd;
	std::string	filePath;
	long				fileSize;
	FileOp			fileOp;
	int					rspStatus;

	FileData(std::string const & filePath, int const connFd);
};

enum	ConnectionStatus
{
	Active,
	Idle
};

// One ConnectionData per client (connection) socket

struct	ConnectionData
{
	std::vector<ServerConfig const *>	* portConfigs;
	std::size_t							serverIndex;
	std::size_t							locationIndex;
	std::string							ip;
	std::map<std::string, std::string>	urlData;
	InputOutput							io;
	int									rspStatus;
	ConnectionStatus					status;
	time_t								lastActive;
	time_t								lastRead;
	time_t								lastSend;
	int									handledRequests;
	DIR *								dirListNeedle;
	FileData *							fileData;
	CgiData *							cgiData;
	Request								req;

	/*
	**	Maximum time (seconds) between two read operations in the same request,
	**	as well as between the initial connection and the first read operation.
	*/
	static double const									ReadTimeout;
	/*
	**	Maximum time (seconds) between two send operations in the same response,
	**	as well as between the start of the request processing and the first
	**	send operation to the client.
	*/
	static double const									SendTimeout;
	// Maximum time (seconds) a connection can stay idle.
	static double const									keepAliveTimeout;
	// Maximum requests that can reuse each connection.
	static int const										keepAliveMaxReq;

	ConnectionData(void);
	~ConnectionData(void);

	ServerConfig const *		getServer(void);
	LocationConfig const *	getLocation(void);
	void										setIdle(void);
};
