#include "PostProcessor.hpp"

PostProcessor::PostProcessor(Response & response, FdTable & fdTable,
                            Monitor & monitor) : Processor(fdTable, monitor),
                                                  _response(response)
{}

PostProcessor::~PostProcessor(void)
{}

/*
**  A hash table could be implemented to avoid some file I/O and
**  increase response efficiency.
**
**  If user sets a cgi file as default, the complete path,
**  including the cgi_dir, must be provided in property "default_file"
**  of config file.
*/

bool  PostProcessor::_getFilePath(ConnectionData & connData,
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
    {// Requested file is not cgi
      if (loc->upload_dir != "")
      {
        /*
        **  Check for Unsupported Media Type has already been done
        **  in the request validation part.
        */
        filePath = loc->upload_dir + '/' + fileName->second;
        return (true);
      }
    }
    else if (loc->cgi_dir != "")
    {// Requested file is cgi, and cgi_dir exists
      filePath = loc->cgi_dir + '/' + fileName->second;
      file.open(filePath.c_str());
      if (file.is_open()) //Requested file exists
      {
        file.close();
        return (true);
      }
    }
  }
  return (false);
}

bool  PostProcessor::_isAppend(std::string const & filePath) const
{
  std::ifstream file;

  file.open(filePath.c_str());
  if (file.is_open())
  { //There is a file with the same name.
    file.close();
    return (true);
  }
  return (false);
}

bool  PostProcessor::_launchCGI(ConnectionData & connData, pollfd & socket,
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
  /*
  **  cgiData's addition to connData must be done before adding its associated
  **  fds to monitor, otherwise, if a reallocation is done by monitor,
  **  cgiData's associated pollfd will not be updated, and will point to
  **  a previous allocation of monitor fds, which has been deleted.
  **  The way in which monitor reallocates can be changed in the future to
  **  prevent this condition. Currently, monitor stores all connection fds, and
  **  iterates through them to update its associated cgiData or fileData when
  **  reallocating. An alternative could be to store all cgiData and fileData
  **  instead and update them directly without knowing its associated connection
  **  fd, but that requires to distinguish its type at the moment of updating.
  */
  connData.cgiData = cgiData;
  bodyPair = connData.req.getHeaders().find("BODY");
  if (bodyPair != connData.req.getHeaders().end())
  {
    //Associate write pipe fd with cgi class instance
    this->_fdTable.add(cgiData->getWInPipe(), cgiData, false);
    this->_monitor.add(cgiData->getWInPipe(), POLLOUT, false);
    connData.io.setPayloadSize(bodyPair->second.length());
  }
  else
    cgiData->closeWInPipe();
  //Associate read pipe fd with cgi class instance
  this->_fdTable.add(cgiData->getROutPipe(), cgiData, true);
  //Check POLLIN event of read pipe fd with poll()
  this->_monitor.add(cgiData->getROutPipe(), POLLIN, false);
  return (true);
}

bool  PostProcessor::_openFile(ConnectionData & connData, pollfd & socket,
                                std::string const & filePath,
                                bool const append) const
{
  FileData *  fileData = new FileData(filePath, socket);

  if (append)
  {
    //  Append to existing file.
    if (!this->_response.fileHandler.openFile(fileData->filePath, fileData->fd,
        O_WRONLY | O_APPEND, 0))
    {
      delete fileData;
      return (false);
    }
    fileData->fileOp = Append;
  }
  else
  {
    /*
    **  Create file with Read/Write permission for user, and Read permission
    **  for group and other.
    */
    if (!this->_response.fileHandler.openFile(fileData->filePath, fileData->fd,
        O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))
    {
      delete fileData;
      return (false);
    }
    fileData->fileOp = Create;
  }
  /*
  **  fileData's addition to connData must be done before adding its associated
  **  fds to monitor, otherwise, if a reallocation is done by monitor,
  **  fileData's associated pollfd will not be updated, and will point to
  **  a previous allocation of monitor fds, which has been deleted.
  **  The way in which monitor reallocates can be changed in the future to
  **  prevent this condition. Currently, monitor stores all connection fds, and
  **  iterates through them to update its associated cgiData or fileData when
  **  reallocating. An alternative could be to store all cgiData and fileData
  **  instead and update them directly without knowing its associated connection
  **  fd, but that requires to distinguish its type at the moment of updating.
  */
  connData.fileData = fileData;
  this->_monitor.add(fileData->fd, POLLOUT, false);
  this->_fdTable.add(fileData->fd, fileData);
  return (true);
}

bool  PostProcessor::start(pollfd & socket, int & error) const
{
  ConnectionData &  connData = this->_fdTable.getConnSock(socket.fd);
  std::string       filePath;
  std::string       cgiInterpreterPath;

  if (!this->_getFilePath(connData, filePath))
  {
    filePath.clear();
    error = 400; // Bad Request
    return (false);
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
      if(!this->_openFile(connData, socket, filePath,
          this->_isAppend(filePath)))
      {
        error = 500; // Internal Server Error
        return (false);
      }
    }
  }
  return (true);
}
