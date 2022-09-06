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
  bool  _launchCGI(ConnectionData & connData, pollfd & socket,
                    std::string const & interpreterPath,
                    std::string const & scriptPath) const;
  bool  _removeFile(pollfd & socket, ConnectionData & connData,
                    std::string const & filePath) const;
  DeleteProcessor(void);

public:

  DeleteProcessor(Response & response, FdTable & fdTable, Monitor & monitor);
  ~DeleteProcessor(void);

  bool  start(pollfd & socket, int & error) const;

};
