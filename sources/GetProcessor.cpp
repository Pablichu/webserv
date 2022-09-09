#include "GetProcessor.hpp"

GetProcessor::GetProcessor(Response & response, FdTable & fdTable,
                            Monitor & monitor) : Processor(fdTable, monitor),
                                                  _response(response)
{}

GetProcessor::~GetProcessor(void)
{}

/*
**  A hash table could be implemented to avoid some file I/O and
**  increase response efficiency.
**
**  If user sets a cgi file as default, the complete path,
**  including the cgi_dir, must be provided in property "default_file"
**  of config file.
*/

bool  GetProcessor::_getFilePath(ConnectionData & connData,
                                  std::string & filePath) const
{
  LocationConfig const *                        loc = connData.getLocation();
  std::map<std::string, std::string>::iterator  fileName;
  std::ifstream                                 file;

  filePath = loc->root + '/';
  fileName = connData.urlData.find("FILENAME");
  if (fileName != connData.urlData.end()) //FileName present in request uri
  {
    if (this->_getCgiInterpreter(connData,
        connData.urlData.find("FILETYPE")->second) == "") //Requested file is not cgi
      filePath.append(fileName->second);
    else if (loc->cgi_dir != "") //Requested file is cgi, and cgi_dir exists
      filePath = loc->cgi_dir + '/' + fileName->second;
    else //Requested file is cgi, but no cgi_dir exists in this location
      return (false);
  }
  else //FileName not present in request uri
    filePath.append(loc->default_file);
  file.open(filePath.c_str());
  if (file.is_open()) //Requested file exists
    file.close();
  else //Requested file does not exist
    return (false);
  return (true);
}

bool  GetProcessor::_launchCGI(ConnectionData & connData, pollfd & socket,
                                std::string const & interpreterPath,
                                std::string const & scriptPath) const
{
  CgiData *                                           cgiData;
  std::map<std::string, std::string>::const_iterator  bodyPair;

  cgiData = new CgiData(socket, interpreterPath, scriptPath);
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
  connData.cgiData = cgiData;
  return (true);
}

bool  GetProcessor::_openFile(ConnectionData & connData, pollfd & socket,
                              std::string const & filePath) const
{
  FileData *  fileData = new FileData(filePath, socket);

  if (!this->_response.fileHandler.openFile(fileData->filePath, fileData->fd,
                                            O_RDONLY, 0))
  {
    delete fileData;
    return (false);
  }
  this->_monitor.add(fileData->fd, POLLIN);
  this->_fdTable.add(fileData->fd, fileData);
  connData.fileData = fileData;
  return (true);
}

bool  GetProcessor::start(pollfd & socket, int & error) const
{
  ConnectionData &        connData = this->_fdTable.getConnSock(socket.fd);
  LocationConfig const *  loc = connData.getLocation();
  std::string             filePath;
  std::string             cgiInterpreterPath;

  if (!this->_getFilePath(connData, filePath))
  {
    filePath.clear();
    if (loc->dir_list == true && !connData.urlData.count("FILENAME"))
    {
      this->_response.buildDirList(socket, connData,
        connData.urlData.find("PATH")->second, loc->root);
    }
    else
    {
      error = 404; // Not Found
      return (false);
    }
  }
  else
  {
    cgiInterpreterPath = this->_getCgiInterpreter(connData,
      filePath.substr(filePath.rfind('.') + 1));
    if (cgiInterpreterPath != "")
    {
      if (!this->_launchCGI(connData, socket, cgiInterpreterPath, filePath))
      {
        error = 500; // Internal Server Error
        return (false);
      }
    }
    else
    {
      if(!this->_openFile(connData, socket, filePath))
      {
        error = 500; // Internal Server Error
        return (false);
      }
    }
  }
  return (true);
}
