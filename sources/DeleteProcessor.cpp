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

  fileName = connData.urlData.find("FILENAME");
  if (fileName != connData.urlData.end()) //FileName present in request uri
  {
    if (this->_getCgiInterpreter(connData,
        connData.urlData.find("FILETYPE")->second) == "")
    { //Requested file is not cgi
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

bool  DeleteProcessor::_launchCGI(ConnectionData & connData, int const fd,
                                  std::string const & interpreterPath,
                                  std::string const & scriptPath) const
{
  CgiData *                                           cgiData;
  std::map<std::string, std::string>::const_iterator  bodyPair;

  cgiData = new CgiData(fd, interpreterPath, scriptPath);
  if (!this->_response.cgiHandler.initPipes(*cgiData,
      *this->_response.cgiHandler.getEnv(connData.req.getHeaders(),
                                          connData.urlData,
                                          connData.getLocation()->root,
                                          connData.ip)))
  {
    delete cgiData;
    return (false);
  }
  //Set non-blocking pipe fds
  if (fcntl(cgiData->getWInPipe(), F_SETFL, O_NONBLOCK)
      || fcntl(cgiData->getROutPipe(), F_SETFL, O_NONBLOCK))
  {
    std::cerr << "Could not set non-blocking pipe fds" << std::endl;
    cgiData->closeWInPipe();
    cgiData->closeROutPipe();
    delete cgiData;
    return (false);
  }
  connData.cgiData = cgiData;
  bodyPair = connData.req.getHeaders().find("BODY");
  if (bodyPair != connData.req.getHeaders().end())
  {
    //Associate write pipe fd with cgi class instance
    this->_fdTable.add(cgiData->getWInPipe(), cgiData, false);
    this->_monitor.add(cgiData->getWInPipe(), POLLOUT);
    connData.io.setPayloadSize(bodyPair->second.length());
  }
  else
    cgiData->closeWInPipe();
  //Associate read pipe fd with cgi class instance
  this->_fdTable.add(cgiData->getROutPipe(), cgiData, true);
  //Check POLLIN event of read pipe fd with poll()
  this->_monitor.add(cgiData->getROutPipe(), POLLIN);
  return (true);
}

bool  DeleteProcessor::_removeFile(int const fd, ConnectionData & connData,
                                    std::string const & filePath) const
{
  std::string content;

  if (!this->_response.fileHandler.removeFile(filePath))
    return (false);
  //Build success response directly
  this->_response.buildDeleted(fd, connData);
  return (true);
}

bool  DeleteProcessor::start(int const fd, int & error) const
{
  ConnectionData &  connData = this->_fdTable.getConnSock(fd);
  std::string       filePath;
  std::string       cgiInterpreterPath;

  if (!this->_getFilePath(connData, filePath))
  {
    filePath.clear();
    error = 404; // Not Found
    return (false);
  }
  else
  {
    cgiInterpreterPath = this->_getCgiInterpreter(connData,
                          utils::getFileExtension(filePath));
    if (cgiInterpreterPath != "")
    {
      if (!this->_launchCGI(connData, fd, cgiInterpreterPath, filePath))
      {
        error = 500; // Internal Server Error
        return (false);
      }
    }
    else
    {
      if(!this->_removeFile(fd, connData, filePath))
      {
        error = 500; // Internal Server Error
        return (false);
      }
    }
  }
  return (true);
}
