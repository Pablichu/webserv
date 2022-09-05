#pragma once

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <dirent.h>

#include "Data.hpp"
#include "FdTable.hpp"
#include "Monitor.hpp"
#include "FileHandler.hpp"
#include "CgiHandler.hpp"
#include "GetProcessor.hpp"
#include "DeleteProcessor.hpp"
#include "PostProcessor.hpp"
#include "HttpInfo.hpp"

/*
**	Circular dependency between Response and GetProcessor
**	forced to add it here as a reference or pointer instead
**	of declaring it in memory.
*/

class	GetProcessor;
class	DeleteProcessor;
class PostProcessor;

class	Response
{

private:

	FdTable &					_fdTable;
	Monitor &					_monitor;
	GetProcessor &		_getProcessor;
	DeleteProcessor &	_deleteProcessor;
	PostProcessor &		_postProcessor;

	void	_buildResponse(ConnectionData & connData, std::string const & content);
	Response(void);

public:

	FileHandler	fileHandler;
  CgiHandler  cgiHandler;	

	Response(FdTable & fdTable, Monitor & monitor);
	~Response(void);

	void	buildRedirect(ConnectionData & connData, std::string const & url,
											int const code);
	void	buildDeleted(ConnectionData & connData);
	void	buildUploaded(ConnectionData & connData, std::string const & url);
	void	buildDirList(ConnectionData & connData, std::string const & uri,
											std::string const & root);
	void	buildError(ConnectionData & connData, int const error);
	bool	process(pollfd & socket, int & error);
	void	sendError(pollfd & socket, int error);
	bool	sendData(int const sockFd, ConnectionData & connData);

};
