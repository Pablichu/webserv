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

  fileName = connData.urlData.find("FileName");
  if (fileName != connData.urlData.end()) //FileName present in request uri
  { //Provisional. Need to check multiple cgi extensions if necessary (php, cgi, py, etc.)
    if (connData.urlData.find("FileType")->second != ".cgi")
    {// Requested file is not cgi
      if (loc->upload_dir != "")
      {
        /*
        **  Check for Unsupported Media Type has already been done
        **  in the request validation part.
        */
        filePath = loc->upload_dir + '/' + fileName->second;
        file.open(filePath.c_str());
        if (file.is_open())
        { //There is a file with the same name.
          /*
          ** POST can create a new resource or append to an existing one.
          */
          file.seekg(0, file.end);
          if (std::atoi(connData.req.getPetit("Content-Length").c_str())
              <= file.tellg())
          {
            /*
            **  It cannot be an append.
            **
            **  An append is considered as uploading the existing content of
            **  a file plus new content.
            */
            file.close();
            return (false);
          }
          else
          {
            /*
            **  It could be an append.
            **
            **  Check if the request payload contains all the existent file's
            **  content. If that's the case, it is an append.
            */
            file.close();
            return (false);
          }
        }
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

bool  PostProcessor::_launchCGI(ConnectionData & connData, int const sockFd,
                                std::string const & filePath) const
{
  CgiData * cgiData;

  cgiData = new CgiData(sockFd, filePath);
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
  if (connData.req.getHeaders().find("Body") != connData.req.getHeaders().end())
  {
    //Associate write pipe fd with cgi class instance
    this->_fdTable.add(cgiData->getWInPipe(), cgiData, false);
    this->_monitor.add(cgiData->getWInPipe(), POLLOUT);
    connData.rspSize = connData.req.getPetit("Body").length();
  }
  else
    close(cgiData->getWInPipe());
  //Associate read pipe fd with cgi class instance
  this->_fdTable.add(cgiData->getROutPipe(), cgiData, true);
  //Check POLLIN event of read pipe fd with poll()
  this->_monitor.add(cgiData->getROutPipe(), POLLIN);
  connData.cgiData = cgiData;
  return (true);
}

bool  PostProcessor::_openFile(ConnectionData & connData, int const sockFd,
                              std::string const & filePath) const
{
  FileData *  fileData = new FileData(filePath, sockFd);

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
  this->_monitor.add(fileData->fd, POLLOUT);
  this->_fdTable.add(fileData->fd, fileData);
  connData.fileData = fileData;
  return (true);
}

bool  PostProcessor::start(int const sockFd, int & error) const
{
  ConnectionData &        connData = this->_fdTable.getConnSock(sockFd);
  LocationConfig const *  loc = connData.getLocation();
  std::string             filePath;

  if (!this->_getFilePath(connData, filePath))
  {
    if (filePath.find(loc->upload_dir) != std::string::npos)
    { //A file with the same name already exists
      this->_response.buildRedirect(connData, connData.req.getPetit("Path"),
                                    303); // See Other
      filePath.clear();
      return (true);
    }
    filePath.clear();
    error = 400; // Bad Request
    return (false);
  }
  else
  {
    if (filePath.rfind(".cgi") != std::string::npos) //Provisional. TODO: substr and able to check multiple cgi extensions if necessary
    {
      if (!this->_launchCGI(connData, sockFd, filePath))
      {
        error = 500; // Internal Server Error
        return (false);
      }
    }
    else
    {
      if(!this->_openFile(connData, sockFd, filePath))
      {
        error = 500; // Internal Server Error
        return (false);
      }
      connData.rspStatus = 200; //Provisional
    }
  }
  return (true);
}
