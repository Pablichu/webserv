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
  std::string   _getCgiInterpreter(ConnectionData & conn,
                        std::string const & extension) const;
  virtual bool  _getFilePath(ConnectionData & connData,
                              std::string & filePath) const = 0;
  virtual bool  _launchCGI(ConnectionData & conn, int const fd,
                            std::string const & interpreterPath,
                            std::string const & scriptPath) const = 0;

public:

  Processor(FdTable & fdTable, Monitor & monitor);
  virtual ~Processor(void);

  virtual bool  start(int const fd, int & error) const = 0;

};
