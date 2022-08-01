#pragma once

#include "webserv.hpp"

class FileHandler
{
public:

  FileHandler(void);
  ~FileHandler(void);

  bool	openFile(std::string const & filePath, int & fd);
	bool	readFileFirst(int const fd, ConnectionData & connData);
	bool	readFileNext(int const fd, ConnectionData & connData);
};
