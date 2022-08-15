#pragma once

#include <fcntl.h>

#include "Data.hpp"
#include "HttpInfo.hpp"
#include "utils.hpp"

class FileHandler
{
public:

  FileHandler(void);
  ~FileHandler(void);

  bool	openFile(std::string const & filePath, int & fd, int const flags,
                  int const mode);
  bool  removeFile(std::string const & filePath) const;
	bool	readFileFirst(int const fd, ConnectionData & connData);
	bool	readFileNext(int const fd, ConnectionData & connData);
  bool  writeFile(int const fd, ConnectionData & connData) const;
};
