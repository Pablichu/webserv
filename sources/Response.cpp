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

void  Response::_buildResponse(ConnectionData & connData,
                                std::string const & content)
{
  connData.buff.replace(0, content.size(), content);
  connData.buffSize = connData.buff.find('\0');
  connData.rspSize = connData.buffSize;
  return ;
}

/*
**  url can be absolute or relative. Administrator decides in config file.
*/

void  Response::buildRedirect(ConnectionData & connData,
                              std::string const & url, int const code)
{
  std::string content;

  content = "HTTP/1.1 " + utils::toString<int>(code) + ' '
            + HttpInfo::statusCode.find(code)->second + "\r\n";
  content += "Date: " + utils::getDate() + "\r\n";
  content += "Location: " + url + "\r\n\r\n";
  this->_buildResponse(connData, content);
  return ;
}

void  Response::buildDeleted(ConnectionData & connData)
{
  std::string content;

  content = "HTTP/1.1 200 OK\r\n";
  content.append("Date: " + utils::getDate() + "\r\n");
  content.append("Content-type: text/html; charset=utf-8\r\n\r\n");
  content.append("<html><body><h1>File deleted.</h1></body></html>");
  this->_buildResponse(connData, content);
  return ;
}

void  Response::buildCreated(ConnectionData & connData, std::string const & url)
{
  std::string content;

  content = "HTTP/1.1 201 " + HttpInfo::statusCode.find(201)->second + "\r\n";
  content.append("Date: " + utils::getDate() + "\r\n");
  content += "Location: " + url + "\r\n";
  content.append("Content-type: text/html; charset=utf-8\r\n\r\n");
  content.append("<html><body><h1>File created.</h1></body></html>");
  this->_buildResponse(connData, content);
  return ;
}

void  Response::buildDirList(ConnectionData & connData, std::string const & uri,
                              std::string const & root)
{
  DIR *           dir;
  struct dirent * elem;
  std::string     content;

  content = "HTTP/1.1 200 OK\r\n";
  content.append("Date: " + utils::getDate() + "\r\n");
  content.append("Content-type: text/html; charset=utf-8\r\n\r\n");
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
      //Check to ensure the closing html tags will fit in the buffer
      if (content.size() >= ConnectionData::buffCapacity - 25)
      {
        content.erase(content.rfind("<a href"), std::string::npos);
        break ;
      }
    }
    closedir(dir);
  }
  content.append("</pre><hr></body></html>");
  this->_buildResponse(connData, content);
  return ;
}

void  Response::buildError(ConnectionData & connData, int const error)
{
  std::string const errorCode = utils::toString<int>(error);
  std::string const errorDescription = HttpInfo::statusCode.find(error)->second;
  std::string       content;

  content = "HTTP/1.1 " + errorCode + ' ' + errorDescription + "\r\n";
  content.append("Date: " + utils::getDate() + "\r\n");
  content.append("Content-type: text/html; charset=utf-8\r\n\r\n");
  content.append("<html><head><title>");
  content.append(errorCode + ' ' + errorDescription);
  content.append("</title></head><body><h1>");
  content.append(errorCode + ' ' + errorDescription);
  content.append("</h1></body></html>");
  this->_buildResponse(connData, content);
  return ;
}

bool  Response::process(int const sockFd, int & error)
{
  ConnectionData &        connData = this->_fdTable.getConnSock(sockFd);
  LocationConfig const *  loc = connData.getLocation();
  std::string const       reqMethod = connData.req.getPetit("Method");

  if (reqMethod == "GET")
  {
    if (loc->redirection != "")
      this->buildRedirect(connData, loc->redirection, 301); //Moved Permanently
    if (!this->_getProcessor.start(sockFd, error))
      return (false);
  }
  else if (reqMethod == "POST")
  {
    if (loc->redirection != "")
      this->buildRedirect(connData, loc->redirection, 308); //Permanent Redirect
    if (!this->_postProcessor.start(sockFd, error))
      return (false);
  }
  else //Delete
  {
    if (loc->redirection != "")
      this->buildRedirect(connData, loc->redirection, 301); //Moved Permanently
    if (!this->_deleteProcessor.start(sockFd, error))
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

void  Response::sendError(int const socket, int error)
{
  ConnectionData &  connData = this->_fdTable.getConnSock(socket);
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
  this->buildError(connData, error);
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
	this->bytesSent = send(sockFd, &connData.buff[0], connData.buffSize, 0);
	if (this->bytesSent <= 0)
	{
		std::cout << "Could not send data to client." << std::endl;
		return (false);
	}
	connData.totalBytesSent += this->bytesSent;
	if (this->bytesSent == connData.buffSize)
	{
		connData.buff.replace(0, connData.buffSize, 1, '\0');
		connData.buffSize = 0;
	}
	else
		connData.buffOffset = this->bytesSent; //Changed this->bytesRead for this->bytesSent. Check if it is correct
	return (true);
}
