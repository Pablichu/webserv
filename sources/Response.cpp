#include "Response.hpp"

std::string const Response::protocol = "HTTP/1.1";

InitStatusCode::InitStatusCode(void)
{
  this->m.insert(
    std::pair<int const, std::string const>(200, "OK"));
  this->m.insert(
    std::pair<int const, std::string const>(201, "Created"));
  this->m.insert(
    std::pair<int const, std::string const>(301, "Moved Permanently"));
  this->m.insert(
    std::pair<int const, std::string const>(308, "Permanent Redirect"));
  this->m.insert(
    std::pair<int const, std::string const>(400, "Bad Request"));
  this->m.insert(
    std::pair<int const, std::string const>(404, "Not Found"));
  this->m.insert(
    std::pair<int const, std::string const>(405, "Method Not Allowed"));
  this->m.insert(
    std::pair<int const, std::string const>(413, "Payload Too Large"));
  this->m.insert(
    std::pair<int const, std::string const>(415, "Unsupported Media Type"));
  this->m.insert(
    std::pair<int const, std::string const>(500, "Internal Server Error"));
  this->m.insert(
    std::pair<int const, std::string const>(505, ""));
}

std::map<int const, std::string const> const
  Response::statusCode(InitStatusCode().m);

InitContentType::InitContentType(void)
{
  this->m.insert(
    std::pair<std::string const, std::string const>(".css", "text/css"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".gif", "image/gif"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".html", "text/html"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".htm", "text/html"));
  this->m.insert(
    std::pair<std::string const, std::string const>(
      ".ico", "image/vnd.microsoft.icon"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".jpeg", "image/jpeg"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".jpg", "image/jpeg"));
  this->m.insert(
    std::pair<std::string const, std::string const>(
      ".js", "application/javascript"));
  this->m.insert(
    std::pair<std::string const, std::string const>(
      ".json", "application/json"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".mp3", "audio/mpeg"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".png", "image/png"));
  this->m.insert(
    std::pair<std::string const, std::string const>(
      ".rar", "application/vnd.rar"));
  this->m.insert(
    std::pair<std::string const, std::string const>(
      ".tar", "application/x-tar"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".txt", "text/plain"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".wav", "audio/wav"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".xml", "application/xml"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".zip", "application/zip"));
  this->m.insert(
    std::pair<std::string const, std::string const>(
      ".7z", "application/x-7z-compressed"));
}

std::map<std::string const, std::string const> const
  Response::contentType(InitContentType().m);

Response::Response(void) {}

Response::~Response(void) {}

void  Response::_buildResponse(ConnectionData & connData,
                                std::string const & content)
{
  connData.rspBuff.replace(0, content.size(), content);
  connData.rspBuffSize = connData.rspBuff.find('\0');
  connData.rspSize = connData.rspBuffSize;
  return ;
}

/*
**  url can be absolute or relative. Administrator decides in config file.
*/

void  Response::buildRedirect(ConnectionData & connData,
                              std::string const & url)
{
  std::string content;

  content = "HTTP/1.1 301 " + Response::statusCode.find(301)->second + '\n';
  content += "Location: " + url + "\n\n";
  this->_buildResponse(connData, content);
  return ;
}

void  Response::buildDirList(ConnectionData & connData, std::string const & uri,
                              std::string const & root)
{
  DIR *           dir;
  struct dirent * elem;
  std::string     content;

  content = "HTTP/1.1 200 OK\n";
  content.append("Content-type: text/html; charset=utf-8\n\n");
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
      if (content.size() >= ConnectionData::rspBuffCapacity - 25)
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
  std::string const errorDescription = Response::statusCode.find(error)->second;
  std::string       content;

  content = "HTTP/1.1 " + errorCode + ' ' + errorDescription + '\n';
  content.append("Content-type: text/html; charset=utf-8\n\n");
  content.append("<html><head><title>");
  content.append(errorCode + ' ' + errorDescription);
  content.append("</title></head><body><h1>");
  content.append(errorCode + ' ' + errorDescription);
  content.append("</h1></body></html>");
  this->_buildResponse(connData, content);
  return ;
}


/*
**	Sends read file content to client socket. It may be called more than once
**	if file size is bigger than read buffer size.
*/

bool	Response::sendFile(int const sockFd, ConnectionData & connData)
{
	this->bytesSent = send(sockFd, &connData.rspBuff[0], connData.rspBuffSize, 0);
	if (this->bytesSent <= 0)
	{
		std::cout << "Could not send data to client." << std::endl;
		return (false);
	}
	connData.totalBytesSent += this->bytesSent;
	if (this->bytesSent == connData.rspBuffSize)
	{
		connData.rspBuff.replace(0, connData.rspBuffSize, 1, '\0');
		connData.rspBuffSize = 0;
	}
	else
		connData.rspBuffOffset = this->bytesSent; //Changed this->bytesRead for this->bytesSent. Check if it is correct
	return (true);
}
