#pragma once

#include "Processor.hpp"
#include "Response.hpp"

/*
**  Could not add Response to Processor because of circular dependency problems.
*/

class Response;

class PostProcessor : public Processor
{

private:

  Response &  _response;

  bool  _getFilePath(ConnectionData & connData, std::string & filePath) const;
  bool  _isAppend(std::string const & filePath) const;
  bool  _launchCGI(ConnectionData & connData, int const sockFd,
                    std::string const & interpreterPath,
                    std::string const & scriptPath) const;
  bool  _openFile(ConnectionData & connData, int const sockFd,
                  std::string const & filePath, bool const append) const;
  PostProcessor(void);

public:

  PostProcessor(Response & response, FdTable & fdTable, Monitor & monitor);
  ~PostProcessor(void);

  bool  start(int const sockFd, int & error) const;

};
