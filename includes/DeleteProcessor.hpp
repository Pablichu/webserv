#pragma once

#include "Processor.hpp"
#include "Response.hpp"

/*
**  Could not add Response to Processor because of circular dependency problems.
*/

class Response;

class DeleteProcessor : public Processor
{

private:

  Response &  _response;

  bool  _getFilePath(ConnectionData & connData, std::string & filePath) const;
  bool  _launchCGI(ConnectionData & connData, int const sockFd,
                    std::string const & interpreterPath,
                    std::string const & scriptPath) const;
  bool  _removeFile(ConnectionData & connData,
                    std::string const & filePath) const;
  DeleteProcessor(void);

public:

  DeleteProcessor(Response & response, FdTable & fdTable, Monitor & monitor);
  ~DeleteProcessor(void);

  bool  start(int const sockFd, int & error) const;

};
