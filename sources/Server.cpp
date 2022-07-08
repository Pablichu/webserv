#include "Server.hpp"

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
  cgiData = new CgiData(socket, connIndex);
  if (!this->_cgiHandler.initPipes(*cgiData, conn,
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
  ConnectionData &            sockData = this->_fdTable.getConnSock(sockPair.first);

  if (!sockData.totalBytesRead)
  {
    if (!this->_response.readFileFirst(fd, sockData))
    {
      this->_endConnection(sockPair.first, sockPair.second);
      return (false);
    }
  }
  else
  {
    if (!this->_response.readFileNext(fd, sockData))
    {
      close(fd);
      this->_monitor.remove(index);
	  this->_fdTable.remove(fd);
      this->_endConnection(sockPair.first, sockPair.second);
      return (false);
    }
  }
  if (static_cast<long>(sockData.totalBytesRead) == sockData.fileSize)
  {
    close(fd);
    this->_monitor.remove(index);
	this->_fdTable.remove(fd);
  }
  this->_monitor[sockPair.second].events = POLLOUT;
  return (true);
}

// Provisional listDir belongs to Response class

std::string Server::_listDir(std::string const & uri,
                              std::string const & root) const
{
  DIR *           dir;
  struct dirent * elem;
  std::string     res;

  res = "HTTP/1.1 200 OK\n";
  res.append("Content-type: text/html; charset=utf-8\n\n");
  res.append("<html><head><title>Index of ");
  res.append(uri);
  res.append("</title></head><body><h1>Index of ");
  res.append(uri);
  res.append("</h1><hr><pre><a href=\"../\">../</a>\n");
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
      res.append("<a href=\"" + uri + '/');
      res.append(elem->d_name);
      res.append("\">");
      res.append(elem->d_name);
      res.append("</a>\n");
      //Check to ensure the closing html tags will fit in the buffer
      if (res.size() >= ConnectionData::rspBuffCapacity - 25)
      {
        res.erase(res.rfind("<a href"), std::string::npos);
        break ;
      }
    }
    closedir(dir);
  }
  res.append("</pre><hr></body></html>");
  return (res);
}

/*
**  Provisional, this should be handled in Response class.
**
**  A hash table could be implemented to avoid some file I/O and
**  increase response efficiency.
*/

void  Server::_getFilePath(ConnectionData & conn) const
{
  ServerConfig const *    serv = (*conn.portConfigs)[conn.serverIndex];
  LocationConfig const *  loc = &serv->location[conn.locationIndex];
  std::string const       path = conn.req.getPetit("Path");
  std::ifstream           file;
  std::string             aux;

  conn.filePath = loc->root + '/';
  //TODO: Save find result to avoid repeating operation after
  if (path.find(".") != std::string::npos)
    conn.filePath.append(path.substr(path.find_last_of("/") + 1,
                    path.length() - path.find_last_of("/") + 1));
  else
    conn.filePath.append(loc->default_file);
  file.open(conn.filePath.c_str());
  if (file.is_open())
    file.close();
  else if (loc->dir_list == true && path.find(".") == std::string::npos)
  {
    aux = this->_listDir(path, loc->root);
    conn.rspBuff.replace(0, aux.size(), aux);
    conn.rspBuffSize = conn.rspBuff.find('\0');
    conn.rspSize = conn.rspBuffSize;
    conn.filePath.clear();
    return ;
  }
  else
    conn.filePath = serv->not_found_page;
  return ;
}

void  Server::_matchLocation(std::vector<LocationConfig> const & locations,
                              std::size_t & index, std::string const & reqUri)
{
  std::size_t                                 uriLen;
  std::vector<LocationConfig>::const_iterator it;
  std::size_t                                 i;

  uriLen = 0;
  for (it = locations.begin(), i = 0; it != locations.end(); ++it, ++i)
  {
    if (reqUri.find(it->uri) == 0)
    {
      if (reqUri.length() > uriLen)
      {
        uriLen = it->uri.length();
        index = i;
      }
    }
  }
  if (uriLen == 0)
    index = 0;
  return ;
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

void  Server::_matchConfig(int socket)
{
  ConnectionData &  sockData = this->_fdTable.getConnSock(socket);

  this->_matchServer(
    *(sockData.portConfigs),
    sockData.serverIndex,sockData.req.getPetit("Host"));
  this->_matchLocation(
    (*sockData.portConfigs)[sockData.serverIndex]->location,
    sockData.locationIndex, sockData.req.getPetit("Path"));
}

/*
**  Depending on the request path, it will:
**
**  1. Open the file that has to be sent.
**  2. Create the pipe that connects a cgi program to the server.
**  3. Create an html listing directory contents.
*/

bool  Server::_prepareResponse(int socket, std::size_t index)
{
  ConnectionData &  sockData = this->_fdTable.getConnSock(socket);

  this->_matchConfig(socket);
  if (sockData.req.getPetit("Path").find(".cgi")
      != std::string::npos) //provisional
  {
    if (!this->_launchCgi(socket, sockData, index))
      return (false);
  }
  else
  {
    this->_getFilePath(sockData);
    if (sockData.filePath.empty()) //Send directory list
    {
      this->_monitor[index].events = POLLOUT;
      return (true);
    }
    if (!this->_response.openFile(sockData.filePath, sockData.fileFd))
      return (false);
    this->_monitor.add(sockData.fileFd, POLLIN);
	this->_fdTable.add(sockData.fileFd, new std::pair<int,std::size_t>(socket, index));
  }
  /*
  **  Do not check for events from this client socket until we receive data
  **  from file or cgi program.
  */
  this->_monitor[index].events = 0;
  return (true);
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
    if (!this->_receiveData(fd))
      return this->_endConnection(fd, index); //TODO: Handle Error
    std::cout << "Data received for "
              << this->_fdTable.getConnSock(fd).req.getPetit("Host")
              << " with path "
              << this->_fdTable.getConnSock(fd).req.getPetit("Path")
              << std::endl;
              //<< this->_connectionSockets[fd].req.getPetit("Path")
    if (!this->_prepareResponse(fd, index))
      this->_endConnection(fd, index);
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
