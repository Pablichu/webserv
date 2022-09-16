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
                    std::string const & locationRoot,
                    std::string const & ip)
{
  std::vector<char *> *                               env;
  std::map<std::string, std::string>::const_iterator  headerIt;
  std::map<std::string, std::string>::const_iterator  urlDataIt;
  std::string                                         content;

  env = new std::vector<char *>();
  content = "AUTH_TYPE";
  this->_addEnvVar(*env, content);
  content = "CONTENT_LENGTH";
  headerIt = reqHeader.find("CONTENT-LENGTH");
  if (headerIt != reqHeader.end())
    content += '=' + headerIt->second;
  this->_addEnvVar(*env, content);
  content = "CONTENT-TYPE";
  headerIt = reqHeader.find("CONTENT-TYPE");
  if (headerIt != reqHeader.end())
   content += '=' + headerIt->second;
  this->_addEnvVar(*env, content);
  content = "GATEWAY_INTERFACE=CGI/1.1";
  this->_addEnvVar(*env, content);
  content = "PATH_INFO";
  urlDataIt = urlData.find("PATH_INFO");
  if (urlDataIt != urlData.end())
    content += '=' + urlDataIt->second;
  this->_addEnvVar(*env, content);
  content = "PATH_TRANSLATED";
  if (urlDataIt != urlData.end())
    content += '=' + locationRoot + urlDataIt->second;
  this->_addEnvVar(*env, content);
  content = "QUERY_STRING";
  urlDataIt = urlData.find("QUERY_STRING");
  if (urlDataIt != urlData.end())
    content += '=' + urlDataIt->second;
  this->_addEnvVar(*env, content);
  content = "REMOTE_ADDR";
  if (!ip.empty())
    content += '=' + ip;
  this->_addEnvVar(*env, content);
  content = "REMOTE_HOST";
  this->_addEnvVar(*env, content);
  //REMOTE_IDENT unset. Not mandatory
  //REMOTE_USER unset. Not mandatory
  content = "REQUEST_METHOD=" + reqHeader.find("METHOD")->second;
  this->_addEnvVar(*env, content);
  /*
  **  The FILENAME key must exist, as the presence of a valid CGI script filename
  **  in the request uri activates CGI processing operations.
  */
  content = "SCRIPT_NAME=" + locationRoot;
  content += '/' + urlData.find("FILENAME")->second;
  this->_addEnvVar(*env, content);
  content = "SERVER_NAME=" + utils::extractHost(reqHeader.find("HOST")->second);
  this->_addEnvVar(*env, content);
  content = "SERVER_PORT=" + utils::extractPort(reqHeader.find("HOST")->second);
  this->_addEnvVar(*env, content);
  content = "SERVER_PROTOCOL=HTTP/1.1";
  this->_addEnvVar(*env, content);
  content = "SERVER_SOFTWARE=42WebServer/1.0";
  this->_addEnvVar(*env, content);
  //Add other Protocol headers.
  for (headerIt = reqHeader.begin(); headerIt != reqHeader.end(); ++headerIt)
  {
    if (HttpInfo::otherHeader.count(headerIt->first))
    {
      content = "HTTP_" + headerIt->first + '=' + headerIt->second;
      std::replace(content.begin(), content.end(), '-', '_');
      this->_addEnvVar(*env, content);
    }
  }
  return (env);
}

bool  CgiHandler::_parseCgiResponse(InputOutput & io,
                                    std::map<std::string, std::string> & header)
{
  std::string const & buff = io.outputStrBuffer();
  std::size_t needle;
  std::size_t aux_needle;
  std::string key;

  aux_needle = 0;
  /*
  **  The condition is equal when none of the characters are found,
  **  having both sides a value of std::string::npos.
  */
  if (buff.find_first_of("\r\n") <= buff.find(':'))
    return (false);
  while(true)
	{
    if (buff[aux_needle] == '\r' || buff[aux_needle] == '\n')
    { //Get body, if it exists
      needle = buff.find_first_of("\r\n", aux_needle);
      if (needle == std::string::npos)
        return (false);
      needle += buff[needle] == '\r' ? 2 : 1;
      // Removes needle buffer bytes and updates bufferSize
      io.addBytesSent(needle);
      if (io.getBufferSize())
        header["BODY"] = buff.substr(0, io.getBufferSize());
      else
        header["BODY"] = "";
      break ;
    }
    needle = buff.find(":", aux_needle);
    if (needle == std::string::npos)
      return (false);
		key = buff.substr(aux_needle, needle - aux_needle);
    std::transform(key.begin(), key.end(), key.begin(), toupper);
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
  io.clear();
  return (true);
}

bool  CgiHandler::_redirect(ConnectionData & connData,
                              std::map<std::string, std::string> const & header,
                              std::string & localPath)
{
  std::map<std::string, std::string>::const_iterator  it;
  std::string                                         data;
  int                                                 statusCode;
  InputOutput &                                       io = connData.io;
  std::string const & location = header.at("LOCATION");

  if (location.find("://") != std::string::npos)
  {//Absolute URI. Send redirection to client, with or without document.
    if (header.at("BODY") != "")
    {
      it = header.find("STATUS");
      if (it == header.end()
          || it->second != "")
        return (false);
      statusCode = atoi(it->second.c_str());
      if (statusCode < 300
          || (statusCode > 304 && statusCode < 307)
          || statusCode > 308)
        return (false);
      data = "HTTP/1.1 " + it->second + ' '
            + HttpInfo::statusCode.at(statusCode) + "\r\n";
      io.pushBack(data);
      data.clear();
      this->_addProtocolHeaders(connData, header);
      this->_addBody(io, header.at("BODY"));
      return (true);
    }
    data = "HTTP/1.1 302 " + HttpInfo::statusCode.at(302) + "\r\n";
    data += "Location: " + header.at("LOCATION") + "\r\n\r\n";
    io.pushBack(data);
    io.setPayloadSize(data.length());
    return (true);
  }
  //Relative URI. Local redirect
  localPath = location;
  return (true);
}

void  CgiHandler::_addProtocolHeaders(ConnectionData & connData,
                              std::map<std::string, std::string> const & header)
{
  std::map<std::string, std::string>::const_iterator  it;
  std::string                                         data;
  InputOutput &                                       io = connData.io;

  for (it = header.begin(); it != header.end(); ++it)
  {
    if (it->first == "STATUS"
        || it ->first == "BODY")
      continue ;
    data = it->first + ": " + it->second + "\r\n";
    io.pushBack(data);
  }
  if (header.find("LOCATION") == header.end())
  {
    utils::addKeepAliveHeaders(data, connData.handledRequests,
                              connData.req.getPetit("CONNECTION") == "close");
    io.pushBack(data);
  }
  io.pushBack("\r\n");
  it = header.find("CONTENT-LENGTH");
  if (it != header.end() && it->second != "")
    io.setPayloadSize(io.getBufferSize() + std::atoi(it->second.c_str()));
  return ;
}

void  CgiHandler::_addBody(InputOutput & io, std::string const & body)
{
  if (!body.length())
    return ;
  io.pushBack(body);
  return ;
}

bool  CgiHandler::_document(ConnectionData & connData,
                            std::map<std::string, std::string> const & header)
{
  std::map<std::string, std::string>::const_iterator  it;
  std::string                                         data;
  InputOutput &                                       io = connData.io;

  it = header.find("STATUS");
  if (it != header.end() && it->second != "")
  {
    if (!HttpInfo::statusCode.count(atoi(it->second.c_str())))
      data = "HTTP/1.1 200 OK\r\n";
    else
    {
      data = "HTTP/1.1 " + it->second + ' '
            + HttpInfo::statusCode.at(atoi(it->second.c_str())) + "\r\n";
    }
    io.pushBack(data);
    return (true);
  }
  else
  {
    if (!header.count("CONTENT-TYPE"))
      return (false);
    data = "HTTP/1.1 200 OK\r\n";
    io.pushBack(data);
  }
  data.clear();
  this->_addProtocolHeaders(connData, header);
  this->_addBody(io, header.at("BODY"));
  return (true);
}

bool
CgiHandler::_reWriteResponse(ConnectionData & connData,
                              std::map<std::string, std::string> const & header,
                              std::string & localPath)
{
  std::map<std::string, std::string>::const_iterator  it;

  it = header.find("LOCATION");
  if (it != header.end() && it->second != "")
  { //Handle redirect
    return (this->_redirect(connData, header, localPath));
  }
  else
  { //Handle document response
    return (this->_document(connData, header));
  }
}

bool  CgiHandler::_buildHttpHeaders(ConnectionData & connData,
                                    std::string & localPath)
{
  std::map<std::string, std::string>  header;

  if (!this->_parseCgiResponse(connData.io, header))
    return (false);
  if (!this->_reWriteResponse(connData, header, localPath))
    return (false);
  return (true);
}

/*
**  -1: Exit failure.
**   0: No exit yet.
**   1: Exit success.
*/

int  CgiHandler::getExitStatus(pid_t const pID)
{
  int res;
  int status;

  errno = 0;
  res = waitpid(pID, &status, WNOHANG);
  if (res < 0)
  {
    std::cout << "waitpid failed with error: " << strerror(errno) << std::endl;
    return (-1);
  }
  else if (res > 0)
  {
    if (WIFEXITED(status))
    {
      if (WEXITSTATUS(status) != EXIT_SUCCESS)
      {
        std::cout << "CGI process exited with error." << std::endl;
        return (-1);
      }
      std::cout << "CGI process exited correctly." << std::endl;
      return (1);
    }
    else
    {
      std::cout << "CGI process was terminated by a signal." << std::endl;
      return (-1);
    }
  }
  return (0);
}

void  CgiHandler::terminateProcess(pid_t const pID)
{
  kill(pID, SIGKILL);
  waitpid(pID, NULL, 0);
  return ;
}

/*
**  Receive data from a cgi pipe's read end.
*/

bool  CgiHandler::receiveData(int rPipe, ConnectionData & connData)
{
  int           len;
  std::string   localPath;
  InputOutput & io = connData.io;
  std::size_t   reserveSpace = 0;

  if (io.getBufferSize() == InputOutput::buffCapacity)
		return (true);
  /*
  **  Reserve space to prevent full buffer in first read, which could lead
  **  to insufficient space to insert the response's HTTP start line.
  **
  **  The longest start line a CGI executed in this server can
  **  currently provide is: HTTP/1.1 505 HTTP Version Not Supported\r\n
  **
  **  If the CGI provides a status code that is not included in HttpInfo
  **  struct, 200 is returned, or the response is considered invalid,
  **  depending on the response type.
  */
  if (io.isFirstRead())
    reserveSpace = 41;
  len = read(rPipe, io.inputBuffer(),
              io.getAvailableBufferSize() - reserveSpace);
  if (len == 0)
  { //EOF
    std::cout << "Pipe write end closed. No more data to read." << std::endl;
    if (io.getPayloadSize() == 0)
      io.setFinishedRead();
    return (true);
  }
  // An error occurred. EAGAIN is not possible, because POLLIN was emitted.
  if (len < 0)
    return (false);
  io.addBytesRead(len);
  if (io.isFirstRead())
  {// First call to receiveData
    if (!this->_buildHttpHeaders(connData, localPath))
      return (false);
    if (localPath != "")
    {
      connData.req.getHeaders()["METHOD"] = "GET";
      connData.req.getHeaders()["PATH"] = localPath;
      UrlParser().parse(localPath, connData.urlData);
    }
  }
  return (true);
}

bool  CgiHandler::sendBody(int wPipe, ConnectionData & connData)
{
  int                 len;
  InputOutput &       io = connData.io;
  std::string &       body = (connData.req.getHeaders())["BODY"];

  io.pushBack(body);
  len = write(wPipe, io.outputBuffer(), io.getBufferSize());
  if (len == 0)
  {
    std::cout << "Could not write to pipe. It may be closed." << std::endl;
    io.clear();
    return (true);
  }
  if (len < 0)
  {
    std::cout << "Pipe error."
              << "POLLOUT was received, and write returned < 0."
              << std::endl;
    return (false);
  }
  io.addBytesSent(len);
  return (true);
}

// execve fails with std::vector<char *> & env

void  CgiHandler::_execProgram(CgiData const & cgiData,
                                std::vector<char *> env)
{
  char *      argv[2 + 1];
  char **     envArr;

  argv[2] = 0;
  argv[0] = const_cast<char *>(cgiData.interpreterPath.c_str());
  argv[1] = const_cast<char *>(cgiData.scriptPath.c_str());
  envArr = new char *[env.size() + 1];
  std::copy(env.begin(), env.end(), envArr);
  envArr[env.size()] = 0;
  dup2(cgiData.inPipe[0], STDIN_FILENO);
  close(cgiData.inPipe[0]);
  dup2(cgiData.outPipe[1], STDOUT_FILENO);
  close(cgiData.outPipe[1]);
  execve(cgiData.interpreterPath.c_str(), argv, envArr);
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
    exit(EXIT_FAILURE);
  }
  else
  { //Parent process
    close(cgiData.inPipe[0]);
    close(cgiData.outPipe[1]);
    this->_deleteEnv(env);
    cgiData.pID = child;
    return (true);
  }
}
