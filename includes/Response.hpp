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

	void	_buildResponse(int const fd, ConnectionData & connData,
												std::string const & content);
	void	_buildChunkedResponse(int const fd, ConnectionData & connData,
												std::string & content,
												std::size_t const endHeadersPos);
	Response(void);

public:

	FileHandler	fileHandler;
  CgiHandler  cgiHandler;	

	Response(FdTable & fdTable, Monitor & monitor);
	~Response(void);

	void	buildRedirect(int const fd, ConnectionData & connData,
											std::string const & url, int const code);
	void	buildDeleted(int const fd, ConnectionData & connData);
	void	buildUploaded(int const fd, ConnectionData & connData,
											std::string const & url);
	void	_addFileLinks(DIR ** dir, std::string & content, std::string const & uri,
											bool const firstCall);
	void	buildDirList(int const fd, ConnectionData & connData,
											std::string const & uri, std::string const & root);
	void  buildDirList(int const fd, ConnectionData & connData);
	void	buildError(int const fd, ConnectionData & connData, int const error);
	bool	process(int const fd, int & error);
	void	sendError(int const fd, int error);

};
