#include "Server.hpp"
#include <cmath>

Server::Server(void)
{
  return ;
}

Server::~Server(void)
{
  return ;
}

bool  Server::_initSocket(int & sock, std::size_t const port)
{
  struct sockaddr_in  address;
  int                 verdad = 1;

  //	Opening socket
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		std::cerr << "Could not open socket" << std::endl;
		return (false);
	}
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &verdad,
                  sizeof(int)))
  {
    std::cerr << "Could not set socket options" << std::endl;
    close(sock);
		return (false);
  }

  //Set non-blocking socket
  if (fcntl(sock, F_SETFL, O_NONBLOCK))
  {
    std::cerr << "Could not set non-blocking socket" << std::endl;
    close(sock);
		return (false);
  }

  //	Setting address struct
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
  for (int i = 0; i < (int)sizeof(address.sin_zero); i++)
		address.sin_zero[i] = 0;

  //	Binding socket
	if (bind(sock, (struct sockaddr *)&(address),
        sizeof(address)))
	{
		std::cerr << "Could not bind socket" << std::endl;
    close(sock);
		return (false);
	}

  //	Start listening to socket
	if (listen(sock, MAX_REQUEST))
	{
		std::cerr << "Could not create socket queue" << std::endl;
    close(sock);
		return (false);
	}
	return (true);
}

/*
**  Obtain one listening socket per each ServerConfig port.
*/

bool  Server::prepare(std::vector<ServerConfig> const & config)
{
  std::vector<ServerConfig>::const_iterator it;
  std::map<std::size_t, int>                addedPorts;
  int                                       sock;

  for (it = config.begin(); it != config.end(); ++it)
  {
    if (addedPorts.insert(std::pair<size_t, int>(it->port, -1)).second == false)
    {
      /*
      **  If port is already bound to a socket, add ServerConfig
      **  as another value to that socket key in ServerConfig.
      */
      this->_fdTable.getListenSockData(addedPorts[it->port]).push_back(&(*it));
    }
    else
    {
      /*
      **  If port is not bound to a socket yet, set up socket
      **  and add socket as key and ServerConfig as value in _listeningSockets.
      */
      if (!this->_initSocket(sock, it->port))
        return (false);
      addedPorts[it->port] = sock;
      this->_fdTable.add(sock, new std::vector<ServerConfig const *>(1, &(*it)));
      //Add listening socket fd to poll array (monitor)
      this->_monitor.add(sock, POLLIN);
    }
  }
  return (true);
}

void  Server::_endConnection(int fd, size_t connIndex)
{
  close(fd);
  this->_monitor.remove(connIndex);
  this->_fdTable.remove(fd);
  return ;
}

bool  Server::_launchCgi(int socket, ConnectionData & conn,
                          std::size_t connIndex)
{
  CgiData * cgiData;
  /*
  **  ADD CGI write and read pipe fds to this->_monitor
  **  and to this->_cgiPipes (Future fd direct address table)
  */
  cgiData = new CgiData(socket, connIndex, conn.filePath);
  if (!this->_cgiHandler.initPipes(*cgiData,
      *this->_cgiHandler.getEnv(conn.req.getHeaders(), conn.urlData, conn.ip)))
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
  //Provisional, need to check writen bytes after each call, as it is non-blocking
  write(cgiData->getWInPipe(), "Hola Mundo!", 11);
  close(cgiData->getWInPipe());
  //Associate read pipe fd with cgi class instance
  this->_fdTable.add(cgiData->getROutPipe(), cgiData);
  //Check POLLIN event of read pipe fd with poll()
  this->_monitor.add(cgiData->getROutPipe(), POLLIN);
  return (true);
}

/*
**  Calls the appropiate Response's readFile method.
*/

bool  Server::_fillFileResponse(int const fd, int const index)
{
  std::pair<int, std::size_t> sockPair = this->_fdTable.getFile(fd);
  ConnectionData &            connData = this->_fdTable.getConnSock(sockPair.first);

  if (!connData.totalBytesRead)
  {
    if (!this->_fileHandler.readFileFirst(fd, connData))
    {
      this->_endConnection(sockPair.first, sockPair.second);
      return (false);
    }
  }
  else
  {
    if (!this->_fileHandler.readFileNext(fd, connData))
    {
      close(fd);
      this->_monitor.remove(index);
	  this->_fdTable.remove(fd);
      this->_endConnection(sockPair.first, sockPair.second);
      return (false);
    }
  }
  if (static_cast<long>(connData.totalBytesRead) == connData.fileSize)
  {
    close(fd);
    this->_monitor.remove(index);
	this->_fdTable.remove(fd);
  }
  this->_monitor[sockPair.second].events = POLLOUT;
  return (true);
}

bool  Server::_openFile(int const socket, int const index,
                          ConnectionData & connData)
{
  if (!this->_fileHandler.openFile(connData.filePath, connData.fileFd))
    return (false);
  this->_monitor.add(connData.fileFd, POLLIN);
  this->_fdTable.add(connData.fileFd,
                      new std::pair<int,std::size_t>(socket, index));
  /*
  **  Do not check for events from this client socket until we receive data
  **  from file or cgi program.
  */
  this->_monitor[index].events = 0;
  return (true);
}

void  Server::_sendListDir(ConnectionData & connData, int const index)
{  
  this->_response.buildDirList(
    connData, connData.urlData.find("Path")->second,
    connData.getLocation()->root
  );
  this->_monitor[index].events = POLLOUT;
  return ;
}

/*
**  Provisional. More than one error page might be available.
**
**  Idea. Create folder for each host:port config where error pages
**  will be stored as errorcode.html. ex: 404.html, 500.html.
**  And search in that folder for errorFolderPath + '/' + errorCode.html,
**  if not found, buildError.
*/

void  Server::_sendError(int const socket, int const index, int error)
{
  ConnectionData &  connData = this->_fdTable.getConnSock(socket);

  if (error == 404) //Not Found
  {
    connData.filePath = connData.getServer()->not_found_page;
    connData.rspStatus = error; //Provisional
    if (!this->_openFile(socket, index, connData))
      error = 500; //An error ocurred while opening file
    else
      return ;
  }
  this->_response.buildError(connData, error);
  this->_monitor[index].events = POLLOUT;
  return ;
}

/*
**  Provisional, this should be handled in Response class.
**
**  A hash table could be implemented to avoid some file I/O and
**  increase response efficiency.
**
**  If user sets a cgi file as default, the complete path,
**  including the cgi_dir, must be provided in proprty "default_file"
**  of config file.
*/

bool  Server::_getFilePath(ConnectionData & connData) const
{
  LocationConfig const *  loc = connData.getLocation();
  std::map<std::string, std::string>::iterator  fileName;
  std::ifstream           file;

  connData.filePath = loc->root + '/';
  fileName = connData.urlData.find("FileName");
  if (fileName != connData.urlData.end()) //FileName present in request uri
  { //Provisional. Need to check multiple cgi extensions if necessary (php, cgi, py, etc.)
    if (connData.urlData.find("FileType")->second != ".cgi") //Requested file is not cgi
      connData.filePath.append(fileName->second);
    else if (loc->cgi_dir != "") //Requested file is cgi, and cgi_dir exists
      connData.filePath = loc->cgi_dir + '/' + fileName->second;
    else //Requested file is cgi, but no cgi_dir exists in this location
      return (false);
  }
  else //FileName not present in request uri
    connData.filePath.append(loc->default_file);
  file.open(connData.filePath.c_str());
  if (file.is_open()) //Requested file exists
    file.close();
  else //Requested file does not exist
    return (false);
  return (true);
}

bool  Server::_prepareGet(int socket, std::size_t index, int & error)
{
  ConnectionData &        connData = this->_fdTable.getConnSock(socket);
  LocationConfig const *  loc = connData.getLocation();

  if (!this->_getFilePath(connData))
  {
    connData.filePath.clear();
    if (loc->dir_list == true && !connData.urlData.count("FileName"))
      this->_sendListDir(connData, index);
    else
    {
      error = 404; // Not Found
      return (false);
    }
  }
  else
  {
    if (connData.filePath.rfind(".cgi") != std::string::npos) //Provisional. TODO: substr and able to check multiple cgi extensions if necessary
    {
      if (!this->_launchCgi(socket, connData, index))
      {
        error = 500; // Internal Server Error
        return (false);
      }
    }
    else
    {
      if(!this->_openFile(socket, index, connData))
      {
        error = 500; // Internal Server Error
        return (false);
      }
      connData.rspStatus = 200; //Provisional
    }
  }
  return (true);
}

bool  Server::_prepareResponse(int socket, std::size_t index, int & error)
{
  ConnectionData &        connData = this->_fdTable.getConnSock(socket);
  LocationConfig const *  loc = connData.getLocation();
  std::string const       reqMethod = connData.req.getPetit("Method");

  if (loc->redirection != "")
  {
    //this->_sendRedirection(loc->redirection);
  }
  else if (reqMethod == "GET")
  {
    if (!this->_prepareGet(socket, index, error))
      return (false);
  }
  else if (reqMethod == "POST")
  {
    /*if (!this->_preparePost(socket, index, error))
      return (false);*/
  }
  else //Delete
  {
    /*if (!this->_prepareDelete(socket, index, error))
      return (false);*/
  }
  return (true);
}

bool  Server::_matchLocation(std::vector<LocationConfig> const & locations,
                              std::size_t & index, std::string const & reqUri)
{
  long                                        uriLen;
  std::vector<LocationConfig>::const_iterator it;
  std::size_t                                 i;

  uriLen = 0;
  for (it = locations.begin(), i = 0; it != locations.end(); ++it, ++i)
  {
    if (reqUri.find(it->uri) == 0)
    {
      if (static_cast<long>(reqUri.length()) > uriLen)
      {
        uriLen = it->uri.length();
        index = i;
      }
    }
  }
  if (static_cast<long>(reqUri.length()) != uriLen)
  {
    //Path not found
    if (! (labs(static_cast<long>(reqUri.length()) - uriLen) == 1
            && reqUri[reqUri.length() - 1] == '/') )
      return (false);
  }
  return (true);
}

void  Server::_matchServer(std::vector<ServerConfig const *> & servers,
                              std::size_t & index, std::string const & reqHost)
{
  int                                         match;
  std::vector<ServerConfig const *>::iterator it;
  std::size_t                                 i;

  match = -1;
  for (it = servers.begin(), i = 0; it != servers.end(); ++it, ++i)
  {
    if ((*it)->server_name.empty() && match == -1)
    {
      index = i;
      match = 0;
    }
    else if ((*it)->server_name.count(reqHost))
    {
      index = i;
      match = 1;
      break ;
    }
  }
  if (match == -1)
    index = 0;
  return ;
}

// Provisional

bool  Server::_matchConfig(int socket)
{
  ConnectionData &  connData = this->_fdTable.getConnSock(socket);
  std::string       path = connData.urlData.find("Path")->second;

  this->_matchServer(
    *(connData.portConfigs),
    connData.serverIndex, connData.req.getPetit("Host"));
  if (connData.urlData.count("FileName"))
    path.erase(path.rfind("/"));
  if (!this->_matchLocation(
      connData.getServer()->location,
      connData.locationIndex, path))
    return (false);
  return (true);
}

/*
**  Depending on the request path, it will:
**
**  1. Open the file that has to be sent.
**  2. Create the pipe that connects a cgi program to the server.
**  3. Create an html listing directory contents.
*/

bool  Server::_validRequest(int socket, int & error)
{
  ConnectionData &  connData = this->_fdTable.getConnSock(socket);

  if (!this->_matchConfig(socket))
  { //Request path did not match any server's location uri
    error = 404; // Not Found
    return (false);
  }
  if (!connData.getLocation()->methods.count(connData.req.getPetit("Method")))
  { //Request method is not allowed in target location
    error = 405; // Method Not Allowed
  }
  //else if (!validBodySize)
    //error = 413; // Payload Too Large
  //else if (!validFileExtension)
    //error = 415; // Unsupported Media Type
  //else if (!validStandardHeaders)
    //error = 400; // Bad Request
  //else if (!validHTTPVersion)
    //error = 505; // HTTP Version Not Supported
  if (error)
    return (false);
  return (true);
}

void  Server::_handleClientRead(int socket, std::size_t index)
{
  int error = 0;

  if (!this->_receiveData(socket))
    return this->_endConnection(socket, index); //TODO: Handle Error
  std::cout << "Data received for "
            << this->_fdTable.getConnSock(socket).req.getPetit("Host")
            << " with path "
            << this->_fdTable.getConnSock(socket).req.getPetit("Path")
            << std::endl;
            //<< this->_connectionSockets[fd].req.getPetit("Path")
  if (!this->_validRequest(socket, error)
      || !this->_prepareResponse(socket, index, error))
    this->_sendError(socket, index, error);
}

/*
**  Sends available data to a client.
**
**  Depending on the request, it will send the requested file
**  or execute a cgi process and send its result.
**
**  It may be called more than once for a single response.
*/

void  Server::_sendData(int socket, std::size_t index)
{
  ConnectionData &  sockData = this->_fdTable.getConnSock(socket);

  if (!this->_response.sendFile(socket, sockData))
  {
    if (this->_fdTable.getType(sockData.fileFd) == File)
    {
      close(sockData.fileFd);
	  this->_fdTable.remove(sockData.fileFd);
      //TODO: delete fileFd from _monitor
    }
    this->_endConnection(socket, index);
  }
}

/*
**  Receive data from a client socket.
*/

bool  Server::_receiveData(int socket)
{
  std::string       reqData;
  std::size_t const buffLen = 500;
  char              buff[buffLen + 1];
  int               len;

  std::fill(buff, buff + buffLen + 1, 0);
  /*
  **  TODO: Handle this recv in a truly non-blocking way.
  */
  while (1)
  {
    len = recv(socket, buff, buffLen, 0);
    if (len == 0)
    {
      std::cout << "Client connection closed unexpectedly." << std::endl;
      return (false);
    }
    if (len < 0)
      break ;
    reqData.append(buff, len);
    std::fill(buff, buff + buffLen, 0);
  }
  this->_fdTable.getConnSock(socket).req.process(reqData);
  UrlParser().parse(this->_fdTable.getConnSock(socket).req.getPetit("Path"),
                    this->_fdTable.getConnSock(socket).urlData);  
  return (true);
}

/*
**  Accept client connections from any virtual server listening socket
**  and add them to the connections array that is passed to poll.
*/

void  Server::_acceptConn(int listenSocket) 
{
  std::vector< ServerConfig const * > * configs;
  struct sockaddr_in                    address;
  socklen_t									            addrLen;
  char                                  ip[INET_ADDRSTRLEN];
  int                                   newConn;

  /*
  **  Get configs vector associated to listeningSocket in advance
  **  to avoid repeated searches in case more than one connection
  **  has been received through this port.
  */
  configs = &this->_fdTable.getListenSockData(listenSocket);
  addrLen = sizeof(address);
  while (true)
  {
    newConn = accept(listenSocket, (struct sockaddr *)&address, &addrLen);
    if (newConn < 0)
    {
      if (errno != EWOULDBLOCK)
      {
        std::cerr << "accept() error" << std::endl;
        continue ;
      }
      break ;
    }
    if (fcntl(newConn, F_SETFL, O_NONBLOCK))
    {
      std::cerr << "Could not set non-blocking data socket" << std::endl;
      close(newConn);
      continue ;
    }
    this->_monitor.add(newConn, POLLIN);
    /*
    **  Add new connection socket as key in _connectionSockets
    **  and configs vector as value.
    */
    this->_fdTable.add(newConn, new ConnectionData);
    this->_fdTable.getConnSock(newConn).portConfigs = configs;
    inet_ntop(address.sin_family, &address.sin_addr, ip, INET_ADDRSTRLEN);
    this->_fdTable.getConnSock(newConn).ip = ip;    
  }
}

/*
**  Handle events from poll fds.
*/

void  Server::_handleEvent(std::size_t index)
{
  int     fd;
  FdType  fdType;

  fd = this->_monitor[index].fd;
  fdType = this->_fdTable.getType(fd);
  if (fdType == ListenSock)
  {
    // New client/s connected to one of listening sockets
    this->_acceptConn(fd);
  }
  else if (this->_fdTable.getType(fd) == Pipe)
  {
    // Read pipe from cgi program is ready to read
    if (!this->_cgiHandler.receiveData(fd, this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket)))
      return ; //Handle Error
    if (this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket).rspSize
		== this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket).totalBytesRead)
    { //All data was received
      this->_monitor.remove(index);
	  this->_fdTable.remove(fd);
      close(fd); //close pipe read fd
      return ;
    }
    // Check again for POLLOUT event of client socket associated to cgi pipe fd
    if (!this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket).totalBytesSent)
      this->_monitor[this->_fdTable.getPipe(fd).connIndex].events = POLLOUT;
  }
  //else if (this->_fileFds.count(fd))
  else if (this->_fdTable.getType(fd) == File)
  {
    // A file fd is ready to read.
    if (!this->_fillFileResponse(fd, index))
      std::cout << "Error reading file" << std::endl;
  }
  else if (this->_monitor[index].revents & POLLIN)
  {
    // Connected client socket is ready to read without blocking
    this->_handleClientRead(fd, index);
  }
  else
  {
    // Connected client socket is ready to write without blocking
    //if (this->_connectionSockets[fd].rspBuffSize)
    if (this->_fdTable.getConnSock(fd).rspBuffSize)
      this->_sendData(fd, index);
    //if (this->_connectionSockets[fd].totalBytesSent == this->_connectionSockets[fd].rspSize)
    if (this->_fdTable.getConnSock(fd).totalBytesSent
		== this->_fdTable.getConnSock(fd).rspSize)
      this->_endConnection(fd, index);
  }
}

/*
**  Start server and call poll() indefinitely to know when clients connect
**  to any virtual server socket.
**
**  Additionally, poll is also used to know when client sockets, cgi pipes
**  and file fds have data to receive or are ready to send data to them.
*/

bool  Server::start(void)
{
  int         numEvents;
  int         handlingCount;

  while (true)
  {
    // TIMEOUT -1 blocks until event is received
    numEvents = poll(this->_monitor.getFds(), this->_monitor.len(), -1);
    if (numEvents < 0)
    {
      std::cerr << "poll() error" << std::endl;
      return (false);
    }
    /* IF TIMEOUT IS NOT SET TO -1 ADD THIS BLOCK TO HANDLE TIMEOUTS
    if (numEvents == 0)
    {
      std::cerr << "poll() timeout" << std::endl;
      continue ;
    }*/
    handlingCount = 0;
    for (std::size_t i = 0; i < this->_monitor.len(); ++i)
    { // INEFFICIENT!! USE kqueue INSTEAD of poll
      if (this->_monitor[i].revents == 0)
        continue;
      this->_handleEvent(i);
      // To stop iterating when total events have been handled
      if (++handlingCount == numEvents)
        break ;
    }
    this->_monitor.purge();
  } 
  return (true);
}
