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

void  Response::_buildResponse(int const fd, ConnectionData & connData,
                                std::string const & content)
{
  InputOutput & io = connData.io;

  io.pushBack(content);
  io.setPayloadSize(io.getBufferSize());
  if (!(this->_monitor[fd].events & POLLOUT))
    this->_monitor[fd].events = POLLIN | POLLOUT;
  return ;
}

void  Response::_buildChunkedResponse(int const fd, ConnectionData & connData,
                                std::string & content,
                                std::size_t const endHeadersPos)
{
  InputOutput & io = connData.io;
  std::size_t const bodySize = content.length() - endHeadersPos;

  content.insert(endHeadersPos, utils::decToHex(bodySize) + "\r\n");
  content.append("\r\n");
  if (io.isFirstRead())
  {
    /*
    **  endHeadersPos's value is still the same at this point because hex length
    **  bytes and \r\n have been inserted after its position, not before.
    */
    content.insert(endHeadersPos - 2, "Transfer-Encoding: chunked\r\n");
  }
  // Last html chunk and 0 chunk are sent at the same time
  if (bodySize && !connData.dirListNeedle)
    content.append("0\r\n\r\n");
  io.pushBack(content);
  if (!connData.dirListNeedle)
    io.setFinishedRead();
  if (!(this->_monitor[fd].events & POLLOUT))
    this->_monitor[fd].events = POLLIN | POLLOUT;
  return ;
}

/*
**  url can be absolute or relative. Administrator decides in config file.
*/

void  Response::buildRedirect(int const fd, ConnectionData & connData,
                              std::string const & url, int const code)
{
  std::string content;

  content = "HTTP/1.1 " + utils::toString<int>(code) + ' '
            + HttpInfo::statusCode.find(code)->second + "\r\n";
  content += "Date: " + utils::getDate() + "\r\n";
  content.append("Server: " + HttpInfo::serverName + "\r\n");
  utils::addKeepAliveHeaders(content, connData.handledRequests,
                              connData.req.getPetit("CONNECTION") == "close");
  content += "Location: " + url + "\r\n\r\n";
  utils::addContentLengthHeader(content, content.length());
  this->_buildResponse(fd, connData, content);
  return ;
}

void  Response::buildDeleted(int const fd, ConnectionData & connData)
{
  std::size_t needle;
  std::string content;

  content = "HTTP/1.1 200 OK\r\n";
  content.append("Date: " + utils::getDate() + "\r\n");
  content.append("Server: " + HttpInfo::serverName + "\r\n");
  utils::addKeepAliveHeaders(content, connData.handledRequests,
                              connData.req.getPetit("CONNECTION") == "close");
  content.append("Content-type: text/html; charset=utf-8\r\n\r\n");
  needle = content.length();
  content.append("<html><body><h1>File deleted.</h1></body></html>");
  utils::addContentLengthHeader(content, needle);
  this->_buildResponse(fd, connData, content);
  return ;
}

void  Response::buildUploaded(int const fd, ConnectionData & connData,
                              std::string const & url)
{
  std::size_t needle;
  std::string content;
  int const   code = connData.fileData->fileOp == Create ? 201 : 200;

  content = "HTTP/1.1 ";
  content.append(utils::toString(code) + ' ');
  content.append(HttpInfo::statusCode.find(code)->second + "\r\n");
  content.append("Date: " + utils::getDate() + "\r\n");
  content.append("Server: " + HttpInfo::serverName + "\r\n");
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
  this->_buildResponse(fd, connData, content);
  return ;
}

/*
**  Passing double pointer for dir in order to be able to modify the
**  pointer address inside the context of this function and apply
**  its changes in the outer context as well.
*/

void  Response::_addFileLinks(DIR ** dir, std::string & content,
                              std::string const & uri, bool const firstCall)
{
  struct dirent * elem;
  std::string     fileName;
  std::size_t     headerSize;

  headerSize = 0;
  if (firstCall)
    headerSize = 28;
  if (!(*dir))
    return ;
  while (true)
  {
    /*
    **  Check to ensure content will fit in the buffer.
    **
    **  16 = size of html characters for each link.
    **  14 = Max. filename length allowed
    **  3 = ... in case name is bigger than 14
    **  1 = / after uri
    **  24 = closing html tags
    **  headerSize reserves 28 bytes because the biggest header that
    **  will be added in the first send is transfer-encoding.
    **  28 = transfer-encoding: chunked\r\n
    **  8 = If chunked response, 4 bytes for hex chunk size
    **  and 4 more for 2 \r\n
    **  5 = if sending chunked response, for last chunk's 0\r\n\r\n
    */
    if ((16 + 14 + 3 + uri.length() + 1)
        >= InputOutput::buffCapacity - 24 - headerSize - 8 - 5)
      return ;
    elem = readdir(*dir);
    if (!elem)
      break ;
    if (elem->d_name[0] == '.') //Skip hidden files
      continue ;
    fileName = elem->d_name;
    if (fileName.length() > 14)
    {
      fileName.erase(14, std::string::npos);
      fileName.append("...");
    }
    content.append("<a href=\"" + uri + '/');
    content.append(fileName);
    content.append("\">");
    content.append(fileName);
    content.append("</a>\n");
  }
  closedir(*dir);
  *dir = 0;
}

/*
**  If first call to buildDirList could not add all files. Call this one
**  from Server to continue sending rest of files in chunks.
*/

void  Response::buildDirList(int const fd, ConnectionData & connData)
{
  std::string         content;
  DIR *               dir = connData.dirListNeedle;
  std::string const & uri = connData.urlData.find("PATH")->second;

  this->_addFileLinks(&dir, content, uri, false);
  if (!dir)
  {// Finished reading files
    content.append("</pre><hr></body></html>");
  }
  this->_buildChunkedResponse(fd, connData, content, 0);
  return ;
}

// First call to get directory listing

void  Response::buildDirList(int const fd, ConnectionData & connData,
                              std::string const & uri, std::string const & root)
{
  DIR *           dir;
  std::size_t     needle;
  std::string     content;

  content = "HTTP/1.1 200 OK\r\n";
  content.append("Date: " + utils::getDate() + "\r\n");
  content.append("Server: " + HttpInfo::serverName + "\r\n");
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
    this->_addFileLinks(&dir, content, uri, true);
  /*
  **  After calling addFileLinks if dir is 0, it means that all file links
  **  could be added to buffer
  */
  if (!dir)
  {
    content.append("</pre><hr></body></html>");
    utils::addContentLengthHeader(content, needle);
    this->_buildResponse(fd, connData, content);
  }
  else
  {
    /*
    **  There might be more file links that did not fit into the buffer.
    **  Send chunked response.
    */
    connData.dirListNeedle = dir;
    this->_buildChunkedResponse(fd, connData, content, needle);
  }
  return ;
}

void  Response::buildError(int const fd, ConnectionData & connData,
                            int const error)
{
  std::string const errorCode = utils::toString<int>(error);
  std::string const errorDescription = HttpInfo::statusCode.find(error)->second;
  std::string       content;

  content = "HTTP/1.1 " + errorCode + ' ' + errorDescription + "\r\n";
  content.append("Date: " + utils::getDate() + "\r\n");
  content.append("Server: " + HttpInfo::serverName + "\r\n");
  utils::addKeepAliveHeaders(content, connData.handledRequests,
                              connData.req.getPetit("CONNECTION") == "close");
  content.append("Content-type: text/html; charset=utf-8\r\n\r\n");
  content.append(this->buildErrorHtml(errorCode, errorDescription));
  this->_buildResponse(fd, connData, content);
  return ;
}

bool  Response::process(int const fd, int & error)
{
  ConnectionData &        connData = this->_fdTable.getConnSock(fd);
  LocationConfig const *  loc = connData.getLocation();
  std::string const       reqMethod = connData.req.getPetit("METHOD");

  if (reqMethod == "GET")
  {
    if (loc->redirection != "")
      this->buildRedirect(fd, connData, loc->redirection, 301); //Moved Permanently
    else if (!this->_getProcessor.start(fd, error))
      return (false);
  }
  else if (reqMethod == "POST")
  {
    if (loc->redirection != "")
      this->buildRedirect(fd, connData, loc->redirection, 308); //Permanent Redirect
    else if (!this->_postProcessor.start(fd, error))
      return (false);
  }
  else //Delete
  {
    if (loc->redirection != "")
      this->buildRedirect(fd, connData, loc->redirection, 301); //Moved Permanently
    else if (!this->_deleteProcessor.start(fd, error))
      return (false);
  }
  return (true);
}

std::string Response::_findErrorPage(std::string const & errorPageDirPath,
                                      int const errorCode)
{
  DIR * dir;
  dirent * elem;
  std::string const targetFileName = utils::toString(errorCode) + ".html";
  std::string errorPagePath = "";

  dir = opendir(errorPageDirPath.c_str());
  if (!dir)
    return (errorPagePath);
  while (true)
  {
    elem = readdir(dir);
    if (!elem)
      break ;
    if (static_cast<std::string>(elem->d_name) == targetFileName)
    {
      errorPagePath = errorPageDirPath + '/' + targetFileName;
      break ;
    }
  }
  closedir(dir);
  return (errorPagePath);
}

/*
**  Each ServerConfig has a directory where custom error pages
**  are stored as errorcode.html. ex: 404.html, 500.html.
**  If errorCode.html is not found in there, buildError.
*/

void  Response::sendError(int const fd, int error)
{
  ConnectionData &  connData = this->_fdTable.getConnSock(fd);
  FileData *        fileData;
  std::string       errorPagePath;

  errorPagePath = this->_findErrorPage(connData.getServer()->error_page_dir,
                                        error);
  if (errorPagePath != "")
  {
    fileData = new FileData(errorPagePath, fd);
    fileData->rspStatus = error;
    if (!this->fileHandler.openFile(fileData->filePath, fileData->fd,
                                    O_RDONLY, 0))
      error = 500; //An error ocurred while opening file
    else
    {
      connData.fileData = fileData;
      this->_fdTable.add(fileData->fd, fileData);
      this->_monitor.add(fileData->fd, POLLIN);
      return ;
    }
  }
  this->buildError(fd, connData, error);
  return ;
}

const std::string	Response::buildErrorHtml(std::string const errorCode, std::string const errorDescription)
{
	std::string		content;
	std::ifstream	in_file(".default/template_error.html");
	std::string		buffer;


	if (!in_file.is_open())
	{
		std::cout << "Cannot access template html" << std::endl;
		content.append("<html><head><title>");
		content.append(errorCode + ' ' + errorDescription);
		content.append("</title></head><body><h1>");
		content.append(errorCode + ' ' + errorDescription);
		content.append("</h1></body></html>\r\n\r\n");
		return content;
	}

	while (true)
	{
		std::getline(in_file, buffer);
		content.append(buffer);
		if (in_file.eof())
		{
			content.append("\r\n\r\n");
			break ;
		}
		else
			content.append("\n");
	}
	in_file.close();
	//replace keys from html
	this->replace(content, "|REPLACE-TITTLE|", errorCode);
	this->replace(content, "|REPLACE-BODY|", errorDescription);
	return content;
}

void	Response::replace(std::string & content, std::string thisStr, std::string forthisStr)
{
	size_t		found;

	while (true)
	{
		found = content.find(thisStr);
		if (found == std::string::npos)
			break ;
		content.replace(found, thisStr.length(), forthisStr);
	}
}
