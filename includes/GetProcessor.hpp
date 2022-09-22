#pragma once

#include "Processor.hpp"
#include "Response.hpp"

/*
**  Could not add Response to Processor because of circular dependency problems.
*/

class Response;

class GetProcessor : public Processor
{

private:

  Response &  _response;

  bool  _getFilePath(ConnectionData & connData, std::string & filePath) const;
  bool  _launchCGI(ConnectionData & connData, int const fd,
                    std::string const & interpreterPath,
                    std::string const & scriptPath) const;
  bool  _openFile(ConnectionData & connData, int const fd,
                  std::string const & filePath) const;
  GetProcessor(void);

public:

  GetProcessor(Response & response, FdTable & fdTable, Monitor & monitor);
  ~GetProcessor(void);

  bool  start(int const fd, int & error) const;

};
