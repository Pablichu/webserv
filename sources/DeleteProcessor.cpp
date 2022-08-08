#include "DeleteProcessor.hpp"

DeleteProcessor::DeleteProcessor(Response & response, FdTable & fdTable,
                                  Monitor & monitor) : Processor(fdTable,
                                                                  monitor),
                                                        _response(response)
{}

DeleteProcessor::~DeleteProcessor(void) {}

bool  DeleteProcessor::_getFilePath(ConnectionData & connData,
                                    std::string & filePath) const
{
  LocationConfig const *                        loc = connData.getLocation();
  std::map<std::string, std::string>::iterator  fileName;
  std::ifstream                                 file;

  fileName = connData.urlData.find("FileName");
  if (fileName != connData.urlData.end()) //FileName present in request uri
  { //Provisional. Need to check multiple cgi extensions if necessary (php, cgi, py, etc.)
    if (connData.urlData.find("FileType")->second != ".cgi") //Requested file is not cgi
    {
      if (loc->upload_dir == "")
        return (false);
      filePath = loc->upload_dir + '/' + fileName->second;
    }
    else if (loc->cgi_dir != "") //Requested file is cgi, and cgi_dir exists
      filePath = loc->cgi_dir + '/' + fileName->second;
    else //Requested file is cgi, but no cgi_dir exists in this location
      return (false);
  }
  else
    return (false); //Maybe 400 Bad Request instead of 404 is better in this case?
  file.open(filePath.c_str());
  if (file.is_open()) //Requested file exists
    file.close();
  else //Requested file does not exist
    return (false);
  return (true);
}

bool  DeleteProcessor::_launchCGI(ConnectionData & connData, int const sockFd,
                                  std::size_t const monitorIndex,
                                  std::string const & filePath) const
{
  CgiData * cgiData;
  /*
  **  ADD CGI write and read pipe fds to this->_monitor
  **  and to this->_cgiPipes (Future fd direct address table)
  */
  cgiData = new CgiData(sockFd, monitorIndex, filePath);
  if (!this->_response.cgiHandler.initPipes(*cgiData,
      *this->_response.cgiHandler.getEnv(connData.req.getHeaders(),
                                          connData.urlData, connData.ip)))
  {
    delete cgiData;
    return (false);
  }
  //Set non-blocking pipe fds
  if (fcntl(cgiData->getWInPipe(), F_SETFL, O_NONBLOCK)
      || fcntl(cgiData->getROutPipe(), F_SETFL, O_NONBLOCK))
  {
    std::cerr << "Could not set non-blocking pipe fds" << std::endl;
    close(cgiData->getWInPipe());
    close(cgiData->getROutPipe());
    delete cgiData;
    return (false);
  }
  //Provisional, need to check writen bytes after each call, as it is non-blocking
  write(cgiData->getWInPipe(), "Hola Mundo!", 11);
  close(cgiData->getWInPipe());
  //Associate read pipe fd with cgi class instance
  this->_fdTable.add(cgiData->getROutPipe(), cgiData);
  //Check POLLIN event of read pipe fd with poll()
  /*
  **  Determine if it makes sense to check for POLLOUT if it is never
  **  going to be used for listening sockets.
  */
  this->_monitor.add(cgiData->getROutPipe(), POLLIN /*| POLLOUT*/);
  cgiData->index = this->_monitor.len() - 1;
  connData.cgiData = cgiData;
  return (true);
}

bool  DeleteProcessor::_removeFile(ConnectionData & connData,
                                    std::string const & filePath) const
{
  std::string content;

  if (!this->_response.fileHandler.removeFile(filePath))
    return (false);
  //Build success response directly
  this->_response.buildDeleted(connData);
  return (true);
}

bool  DeleteProcessor::start(int const sockFd, std::size_t const monitorIndex,
                              int & error) const
{
  ConnectionData &  connData = this->_fdTable.getConnSock(sockFd);
  std::string       filePath;

  if (!this->_getFilePath(connData, filePath))
  {
    filePath.clear();
    error = 404; // Not Found
    return (false);
  }
  else
  {
    if (filePath.rfind(".cgi") != std::string::npos) //Provisional. TODO: substr and able to check multiple cgi extensions if necessary
    {
      if (!this->_launchCGI(connData, sockFd, monitorIndex, filePath))
      {
        error = 500; // Internal Server Error
        return (false);
      }
    }
    else
    {
      if(!this->_removeFile(connData, filePath))
      {
        error = 500; // Internal Server Error
        return (false);
      }
      connData.rspStatus = 200; //Provisional
    }
  }
  return (true);
}
