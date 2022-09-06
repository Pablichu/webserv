#include "Response.hpp"

Response::Response(FdTable & fdTable, Monitor & monitor)
  : _fdTable(fdTable), _monitor(monitor),
    _getProcessor(*(new GetProcessor(*this, _fdTable, _monitor))),
    _deleteProcessor(*(new DeleteProcessor(*this, _fdTable, _monitor))),
    _postProcessor(*(new PostProcessor(*this, _fdTable, _monitor)))
{}

Response::~Response(void)
{
  delete &(this->_getProcessor);
  delete &(this->_deleteProcessor);
  delete &(this->_postProcessor);
  return ;
}

void  Response::_buildResponse(pollfd & socket, ConnectionData & connData,
                                std::string const & content)
{
  InputOutput & io = connData.io;

  io.pushBack(content);
  io.setPayloadSize(io.getBufferSize());
  if (!(socket.events & POLLOUT))
    socket.events = POLLIN | POLLOUT;
  return ;
}

/*
**  url can be absolute or relative. Administrator decides in config file.
*/

void  Response::buildRedirect(pollfd & socket, ConnectionData & connData,
                              std::string const & url, int const code)
{
  std::string content;

  content = "HTTP/1.1 " + utils::toString<int>(code) + ' '
            + HttpInfo::statusCode.find(code)->second + "\r\n";
  content += "Date: " + utils::getDate() + "\r\n";
  utils::addKeepAliveHeaders(content, connData.handledRequests,
                              connData.req.getPetit("CONNECTION") == "close");
  content += "Location: " + url + "\r\n\r\n";
  utils::addContentLengthHeader(content, content.length());
  this->_buildResponse(socket, connData, content);
  return ;
}

void  Response::buildDeleted(pollfd & socket, ConnectionData & connData)
{
  std::size_t needle;
  std::string content;

  content = "HTTP/1.1 200 OK\r\n";
  content.append("Date: " + utils::getDate() + "\r\n");
  utils::addKeepAliveHeaders(content, connData.handledRequests,
                              connData.req.getPetit("CONNECTION") == "close");
  content.append("Content-type: text/html; charset=utf-8\r\n\r\n");
  needle = content.length();
  content.append("<html><body><h1>File deleted.</h1></body></html>");
  utils::addContentLengthHeader(content, needle);
  this->_buildResponse(socket, connData, content);
  return ;
}

void  Response::buildUploaded(pollfd & socket, ConnectionData & connData,
                              std::string const & url)
{
  std::size_t needle;
  std::string content;
  int const   code = connData.fileData->fileOp == Create ? 201 : 200;

  content = "HTTP/1.1 ";
  content.append(utils::toString(code) + ' ');
  content.append(HttpInfo::statusCode.find(code)->second + "\r\n");
  content.append("Date: " + utils::getDate() + "\r\n");
  utils::addKeepAliveHeaders(content, connData.handledRequests,
                              connData.req.getPetit("CONNECTION") == "close");
  content += "Location: " + url + "\r\n";
  content.append("Content-type: text/html; charset=utf-8\r\n\r\n");
  needle = content.length();
  content.append("<html><body><h1>");
  if (code == 201)
    content.append("File created at: ");
  else
    content.append("Content appended to existing file at: ");
  content.append(url);
  content.append("</h1></body></html>");
  utils::addContentLengthHeader(content, needle);
  this->_buildResponse(socket, connData, content);
  return ;
}

void  Response::buildDirList(pollfd & socket, ConnectionData & connData,
                              std::string const & uri, std::string const & root)
{
  DIR *           dir;
  struct dirent * elem;
  std::size_t     needle;
  std::string     content;

  content = "HTTP/1.1 200 OK\r\n";
  content.append("Date: " + utils::getDate() + "\r\n");
  utils::addKeepAliveHeaders(content, connData.handledRequests,
                              connData.req.getPetit("CONNECTION") == "close");
  content.append("Content-type: text/html; charset=utf-8\r\n\r\n");
  needle = content.length();
  content.append("<html><head><title>Index of ");
  content.append(uri);
  content.append("</title></head><body><h1>Index of ");
  content.append(uri);
  content.append("</h1><hr><pre><a href=\"../\">../</a>\n");
  dir = opendir(root.c_str());
  if (dir)
  {
    while (true)
    {
      elem = readdir(dir);
      if (!elem)
        break ;
      if (elem->d_name[0] == '.') //Skip hidden files
        continue ;
      content.append("<a href=\"" + uri + '/');
      content.append(elem->d_name);
      content.append("\">");
      content.append(elem->d_name);
      content.append("</a>\n");
      /*
      **  Check to ensure the closing html tags and Content-Length header
      **  will fit in the buffer
      */
      if (content.size() >= InputOutput::buffCapacity - 46)
      {
        content.erase(content.rfind("<a href"), std::string::npos);
        break ;
      }
    }
    closedir(dir);
  }
  content.append("</pre><hr></body></html>");
  utils::addContentLengthHeader(content, needle);
  this->_buildResponse(socket, connData, content);
  return ;
}

void  Response::buildError(pollfd & socket, ConnectionData & connData,
                            int const error)
{
  std::string const errorCode = utils::toString<int>(error);
  std::string const errorDescription = HttpInfo::statusCode.find(error)->second;
  std::size_t       needle;
  std::string       content;

  content = "HTTP/1.1 " + errorCode + ' ' + errorDescription + "\r\n";
  content.append("Date: " + utils::getDate() + "\r\n");
  utils::addKeepAliveHeaders(content, connData.handledRequests,
                              connData.req.getPetit("CONNECTION") == "close");
  content.append("Content-type: text/html; charset=utf-8\r\n\r\n");
  needle = content.length();
  content.append("<html><head><title>");
  content.append(errorCode + ' ' + errorDescription);
  content.append("</title></head><body><h1>");
  content.append(errorCode + ' ' + errorDescription);
  content.append("</h1></body></html>");
  utils::addContentLengthHeader(content, needle);
  this->_buildResponse(socket, connData, content);
  return ;
}

bool  Response::process(pollfd & socket, int & error)
{
  ConnectionData &        connData = this->_fdTable.getConnSock(socket.fd);
  LocationConfig const *  loc = connData.getLocation();
  std::string const       reqMethod = connData.req.getPetit("METHOD");

  if (reqMethod == "GET")
  {
    if (loc->redirection != "")
      this->buildRedirect(socket, connData, loc->redirection, 301); //Moved Permanently
    else if (!this->_getProcessor.start(socket, error))
      return (false);
  }
  else if (reqMethod == "POST")
  {
    if (loc->redirection != "")
      this->buildRedirect(socket, connData, loc->redirection, 308); //Permanent Redirect
    else if (!this->_postProcessor.start(socket, error))
      return (false);
  }
  else //Delete
  {
    if (loc->redirection != "")
      this->buildRedirect(socket, connData, loc->redirection, 301); //Moved Permanently
    else if (!this->_deleteProcessor.start(socket, error))
      return (false);
  }
  return (true);
}

/*
**  Provisional. More than one error page might be available.
**
**  Idea. Create folder for each host:port config where error pages
**  will be stored as errorcode.html. ex: 404.html, 500.html.
**  And search in that folder for errorFolderPath + '/' + errorCode.html,
**  if not found, buildError.
*/

void  Response::sendError(pollfd & socket, int error)
{
  ConnectionData &  connData = this->_fdTable.getConnSock(socket.fd);
  FileData *        fileData;

  if (error == 404) //Not Found
  {
    fileData = new FileData(connData.getServer()->not_found_page, socket);
    connData.rspStatus = error; //Provisional
    if (!this->fileHandler.openFile(fileData->filePath, fileData->fd,
                                    O_RDONLY, 0))
      error = 500; //An error ocurred while opening file
    else
    {
      this->_monitor.add(fileData->fd, POLLIN);
      this->_fdTable.add(fileData->fd, fileData);
      connData.fileData = fileData;
      return ;
    }
  }
  this->buildError(socket, connData, error);
  return ;
}

/*
**	Sends read file content to client socket. It may be called more than once
**	if file size is bigger than read buffer size.
**
**  Naming it sendData instead of just send because there is
**  a namespace conflict with send of the C Standard Library.
*/

bool	Response::sendData(int const sockFd, ConnectionData & connData)
{
  std::size_t   bytesSent;
  InputOutput & io = connData.io;

	bytesSent = send(sockFd, io.outputBuffer(), io.getBufferSize(), 0);
	if (bytesSent <= 0)
	{
		std::cout << "Could not send data to client." << std::endl;
		return (false);
	}
	io.addBytesSent(bytesSent);
	return (true);
}
