#include "CgiHandler.hpp"

CgiHandler::CgiHandler(void)
{
  return ;
}

CgiHandler::~CgiHandler(void)
{
  return ;
}

void  CgiHandler::_deleteEnv(std::vector<char *> & env)
{
  std::vector<char *>::iterator it;

  for (it = env.begin(); it != env.end(); ++it)
  {
    delete [] *it;
  }
  delete &env;
  return ;
}

void  CgiHandler::_addEnvVar(std::vector<char *> & env, std::string const & var)
{
  char *      aux;
  std::size_t len;

  len = var.length();
  aux = new char [len + 1];
  var.copy(aux, len);
  aux[len] = 0;
  env.push_back(aux);
  return ;
}

//Return vector of CGI environment variables

std::vector<char *> *
CgiHandler::getEnv(std::map<std::string, std::string> const & reqHeader,
                    std::map<std::string, std::string> const & urlData,
                    std::string const & ip)
{
  std::vector<char *> *                               env;
  std::map<std::string, std::string>::const_iterator  headerIt;
  std::map<std::string, std::string>::const_iterator  urlDataIt;
  std::string                                         content;

  env = new std::vector<char *>();
  content = "Auth_Type";
  this->_addEnvVar(*env, content);
  content = "Content_Length";
  headerIt = reqHeader.find("Content-Length");
  if (headerIt != reqHeader.end())
    content += '=' + headerIt->second;
  this->_addEnvVar(*env, content);
  content = "Content_Type";
  headerIt = reqHeader.find("Content-Type");
  if (headerIt != reqHeader.end())
   content += '=' + headerIt->second;
  this->_addEnvVar(*env, content);
  content = "Gateway_Interface=CGI/1.1";
  this->_addEnvVar(*env, content);
  content = "Path_Info";
  urlDataIt = urlData.find("Path_Info");
  if (urlDataIt != urlData.end())
    content += '=' + urlDataIt->second;
  this->_addEnvVar(*env, content);
  content = "Path_Translated";
  if (urlDataIt != urlData.end())
    content += '=' + /* location.root + */ urlDataIt->second;
  this->_addEnvVar(*env, content);
  content = "Query_String";
  urlDataIt = urlData.find("Query_String");
  if (urlDataIt != urlData.end())
    content += '=' + urlDataIt->second;
  this->_addEnvVar(*env, content);
  content = "Remote_Addr";
  if (!ip.empty())
    content += '=' + ip;
  this->_addEnvVar(*env, content);
  content = "Remote_Host";
  this->_addEnvVar(*env, content);
  //REMOTE_IDENT unset. Not mandatory
  //REMOTE_USER unset. Not mandatory
  content = "Request_Method=" + reqHeader.find("Method")->second;
  this->_addEnvVar(*env, content);
  /*
  **  Check if need to add cgi_bin folder path at the front.
  **
  **  This key must exist, as the presence of a valid CGI script filename
  **  in the request uri activates CGI processing operations.
  */
  content = "Script_Name=" + urlData.find("FileName")->second;
  this->_addEnvVar(*env, content);
  content = "Server_Name=" + utils::extractHost(reqHeader.find("Host")->second);
  this->_addEnvVar(*env, content);
  content = "Server_Port=" + utils::extractPort(reqHeader.find("Host")->second);
  this->_addEnvVar(*env, content);
  content = "Server_Protocol=HTTP/1.1";
  this->_addEnvVar(*env, content);
  content = "Server_Software=42WebServer/1.0";
  this->_addEnvVar(*env, content);
  //Protocol headers unset. Not mandatory
  return (env);
}

bool  CgiHandler::_parseCgiResponse(std::string & buff, int const buffSize,
                                    std::map<std::string, std::string> & header)
{
  std::size_t needle;
  std::size_t aux_needle;
  std::string key;

  aux_needle = 0;
  if (buff.find_first_of("\r\n") < buff.find(':'))
    return (false);
  while(true)
	{
    if (buff[aux_needle] == '\r' || buff[aux_needle] == '\n')
    { //Get body, if it exists
      needle = buff.find_first_of("\r\n", aux_needle);
      if (needle == std::string::npos)
        return (false);
      needle += buff[needle] == '\r' ? 2 : 1;
      if (static_cast<std::size_t>(buffSize) > needle + 1)
        header["body"] = buff.substr(needle);
      else
        header["body"] = "";
      break ;
    }
    needle = buff.find(":", aux_needle);
    if (needle == std::string::npos)
      return (false);
		key = buff.substr(aux_needle, needle - aux_needle);
    if (key != "Status" && key != "Location" && key != "Content-Type")
      std::transform(key.begin(), key.end(), key.begin(), tolower);
		needle = buff.find_first_not_of(' ', needle + 1);
    if (needle == std::string::npos)
      return (false);
		aux_needle = buff.find_first_of("\r\n", needle);
		if (aux_needle == std::string::npos)
      return (false);
    //It will only insert unique keys
    header.insert(std::pair<std::string, std::string>(key,
                  buff.substr(needle, aux_needle - needle)));
    aux_needle += buff[aux_needle] == '\r' ? 2 : 1;
	}
  std::fill(&buff[0], &buff[buffSize], 0);
  return (true);
}

bool  CgiHandler::_redirect(std::string & buff,
                              std::map<std::string, std::string> const & header,
                              std::size_t & rspSize, std::string & localPath)
{
  std::map<std::string, std::string>::const_iterator  it;
  std::string const &                                 location = header.at("Location");
  std::string                                         data;

  if (location.find("://") != std::string::npos)
  {//Absolute URI. Send redirection to client, with or without document.
    if (header.at("body") != "")
    {
      it = header.find("Status");
      if (it == header.end()
          || it->second != "")
        return (false);
      data = "HTTP/1.1 " + it->second + ' '
            + HttpInfo::statusCode.at(atoi(it->second.c_str())) + "\r\n";
      buff.replace(0, data.length(), data);
      data.clear();
      this->_addProtocolHeaders(buff, header, rspSize);
      this->_addBody(buff, header.at("body"));
      return (true);
    }
    data = "HTTP/1.1 302 " + HttpInfo::statusCode.at(302) + "\r\n";
    data += "Location: " + header.at("Location") + "\r\n\r\n";
    buff.replace(0, data.length(), data);
    rspSize = data.length();
    return (true);
  }
  //Relative URI. Local redirect
  localPath = location;
  rspSize = 0;
  return (true);
}

void  CgiHandler::_addProtocolHeaders(std::string & buff,
                              std::map<std::string, std::string> const & header,
                              std::size_t & rspSize)
{
  std::map<std::string, std::string>::const_iterator  it;
  std::size_t                                         needle;
  std::string                                         data;

  needle = buff.find('\0');
  for (it = header.begin(); it != header.end(); ++it)
  {
    if (it->first == "Status"
        || it ->first == "body")
      continue ;
    data = it->first + ": " + it->second + "\r\n";
    buff.replace(needle, data.length(), data);
    needle += data.length();
  }
  buff.replace(needle, 2, "\r\n");
  needle += 2;
  it = header.find("content-length");
  if (it != header.end() && it->second != "")
    rspSize = needle + std::atoi(it->second.c_str());
  else
    rspSize = 0;
  return ;
}

void  CgiHandler::_addBody(std::string & buff, std::string const & body)
{
  if (!body.length())
    return ;
  buff.replace(buff.find('\0'), body.length(), body);
  return ;
}

bool  CgiHandler::_document(std::string & buff,
                              std::map<std::string, std::string> const & header,
                              std::size_t & rspSize)
{
  std::map<std::string, std::string>::const_iterator  it;
  std::string                                         data;

  it = header.find("Status");
  if (it != header.end() && it->second != "")
  {
    if (!HttpInfo::statusCode.count(atoi(it->second.c_str())))
      data = "HTTP/1.1 200 OK\r\n";
    else
    {
      data = "HTTP/1.1 " + it->second + ' '
            + HttpInfo::statusCode.at(atoi(it->second.c_str())) + "\r\n";
    }
    buff.replace(0, data.length(), data);
    return (true);
  }
  else
  {
    if (!header.count("Content-Type"))
      return (false);
    data = "HTTP/1.1 200 OK\r\n";
    buff.replace(0, data.length(), data);
  }
  data.clear();
  this->_addProtocolHeaders(buff, header, rspSize);
  this->_addBody(buff, header.at("body"));
  return (true);
}

bool  CgiHandler::_reWriteResponse(std::string & buff,
                                    std::map<std::string, std::string> const & header,
                                    std::size_t & rspSize, std::string & localPath)
{
  std::map<std::string, std::string>::const_iterator  it;

  it = header.find("Location");
  if (it != header.end() && it->second != "")
  { //Handle redirect
    return (this->_redirect(buff, header, rspSize, localPath));
  }
  else
  { //Handle document response
    return (this->_document(buff, header, rspSize));
  }
}

bool  CgiHandler::_buildHttpHeaders(std::string & buff, int const buffSize,
                                    std::size_t & rspSize, std::string & localPath)
{
  std::map<std::string, std::string>  header;

  if (!this->_parseCgiResponse(buff, buffSize, header))
    return (false);
  if (!this->_reWriteResponse(buff, header, rspSize, localPath))
    return (false);
  return (true);
}

/*
**  Receive data from a cgi pipe's read end.
**
**  Need to check Content-length header from cgi response to know when
**  all the data from cgi program has been read.
*/

bool  CgiHandler::receiveData(int rPipe, ConnectionData & connData)
{
  std::size_t	endContent;
  int         len;
  std::string localPath;

  endContent = connData.buffSize;
	if (connData.buffOffset)
	{ //This portion of code is equal to the one in Response::readFileNext
		//Move the content after offset to the front
		connData.buff.replace(connData.buff.begin(),
			connData.buff.begin() + connData.buffOffset,
			&connData.buff[connData.buffOffset]);
		//Fill the content that is duplicated at the back with NULL
		connData.buff.replace(endContent - connData.buffOffset,
			connData.buffOffset, 0);
		//Update endContent
		endContent = endContent - connData.buffOffset;
		//Reset offset
		connData.buffOffset = 0;
	}
  len = read(rPipe, &connData.buff[endContent],
													ConnectionData::buffCapacity - endContent);
  if (len == 0)
  { //EOF
    std::cout << "Pipe write end closed. No more data to read." << std::endl;
    if (!connData.rspSize)
      connData.rspSize = connData.totalBytesRead;
    return (true);
  }
  // An error occurred. EAGAIN is not possible, because POLLIN was emitted.
  if (len < 0)
    return (false);
  if (!connData.totalBytesRead) //First call to receiveData
  {
    if (!this->_buildHttpHeaders(connData.buff, len, connData.rspSize, localPath))
      return (false);
    if (localPath != "")
    {
      connData.req.getHeaders()["Method"] = "GET";
      connData.req.getHeaders()["Path"] = localPath;
      UrlParser().parse(localPath, connData.urlData);
    }
    connData.buffSize = connData.buff.find('\0');
	  connData.totalBytesRead = connData.buffSize; //Fake value, because headers are modified
  }
  else
  {
    connData.buffSize = endContent + len;
	  connData.totalBytesRead += len;
  }
  return (true);
}

bool  CgiHandler::sendBody(int wPipe, ConnectionData & connData)
{
  int                 len;
  std::size_t &       totalBytesSent = connData.totalBytesSent;
  std::string const & body = (connData.req.getHeaders())["Body"];

  len = write(wPipe, &body[totalBytesSent], body.length() - totalBytesSent);
  if (len == 0)
  {
    std::cout << "Could not write to pipe. It may be closed." << std::endl;
    totalBytesSent = 0;
    connData.rspSize = 0;
    return (true);
  }
  if (len < 0)
  {
    std::cout << "Pipe error."
              << "POLLOUT was received, and write returned < 0."
              << std::endl;
    return (false);
  }
  totalBytesSent += len;
  return (true);
}

// execve fails with std::vector<char *> & env

void  CgiHandler::_execProgram(CgiData const & cgiData,
                                std::vector<char *> env)
{
  std::string programPath;
  char *      argv[1 + 1];
  char **     envArr;

  programPath = cgiData.filePath;
  argv[1] = 0;
  argv[0] = const_cast<char *>(programPath.c_str());
  envArr = new char *[env.size() + 1];
  std::copy(env.begin(), env.end(), envArr);
  envArr[env.size()] = 0;
  dup2(cgiData.inPipe[0], STDIN_FILENO);
  close(cgiData.inPipe[0]);
  dup2(cgiData.outPipe[1], STDOUT_FILENO);
  close(cgiData.outPipe[1]);
  execve(programPath.c_str(), argv, envArr);
  delete [] envArr;
}

bool  CgiHandler::initPipes(CgiData & cgiData, std::vector<char *> & env)
{
  pid_t child;

  if (pipe(cgiData.inPipe) == -1
      || pipe(cgiData.outPipe) == -1)
  {
    this->_deleteEnv(env);
    return (false);
  }
  child = fork();
  if (child < 0)
  {
    this->_deleteEnv(env);
    return (false);
  }
  if (!child)
  { //Child process
    close(cgiData.inPipe[1]);
    close(cgiData.outPipe[0]);
    this->_execProgram(cgiData, env);
    std::cout << "exec failed" << std::endl;
    exit(EXIT_FAILURE);
  }
  else
  { //Parent process
    close(cgiData.inPipe[0]);
    close(cgiData.outPipe[1]);
    this->_deleteEnv(env);
    return (true);
  }
}
