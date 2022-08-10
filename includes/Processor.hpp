#pragma once

#include "FdTable.hpp"
#include "Monitor.hpp"
#include "Data.hpp"

class Processor
{

private:

  Processor(void);

protected:

  FdTable &     _fdTable;
	Monitor &     _monitor;
  virtual bool  _getFilePath(ConnectionData & connData,
                              std::string & filePath) const = 0;
  virtual bool  _launchCGI(ConnectionData & conn, int const sockFd,
                            std::string const & filePath) const = 0;

public:

  Processor(FdTable & fdTable, Monitor & monitor);
  virtual ~Processor(void);

  virtual bool  start(int const sockFd, int & error) const = 0;

};
