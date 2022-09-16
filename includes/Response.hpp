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

	void	_buildResponse(pollfd & socket, ConnectionData & connData,
												std::string const & content);
	void	_buildChunkedResponse(pollfd & socket, ConnectionData & connData,
												std::string & content,
												std::size_t const endHeadersPos);
	Response(void);

public:

	FileHandler	fileHandler;
  CgiHandler  cgiHandler;	

	Response(FdTable & fdTable, Monitor & monitor);
	~Response(void);

	void	buildRedirect(pollfd & socket, ConnectionData & connData,
											std::string const & url, int const code);
	void	buildDeleted(pollfd & socket, ConnectionData & connData);
	void	buildUploaded(pollfd & socket, ConnectionData & connData,
											std::string const & url);
	void	_addFileLinks(DIR ** dir, std::string & content, std::string const & uri,
											bool const firstCall);
	void	buildDirList(pollfd & socket, ConnectionData & connData,
											std::string const & uri, std::string const & root);
	void  buildDirList(pollfd & socket, ConnectionData & connData);
	void	buildError(pollfd & socket, ConnectionData & connData, int const error);
	bool	process(pollfd & socket, int & error);
	void	sendError(pollfd & socket, int error);

	const std::string	buildErrorHtml(std::string const errorCode, std::string const errorDescription);
	void				replace(std::string & content, std::string thisStr, std::string forthisStr);
};
