#include "Server.hpp"

ConnectionData::ConnectionData(void) : serverIndex(0), locationIndex(0),
  fileFd(-1), fileSize(0), totalBytesRead(0), totalBytesSent(0), rspSize(0)
{
  return ;
}

ConnectionData::~ConnectionData(void)
{
  return ;
}

ServerConfig const *  ConnectionData::getServer(void)
{
  if (!this->portConfigs->size())
    return (0);
  return ((*this->portConfigs)[this->serverIndex]);
}

LocationConfig  ConnectionData::getLocation(void)
{
  if (!this->portConfigs->size())
    return (LocationConfig());
  return (this->getServer()->location[this->locationIndex]);
}

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
  std::vector<ServerConfig>::const_iterator                   it;
  std::set<std::size_t>                                       addedPorts;
  std::map<int, std::vector<ServerConfig const *> >::iterator jt;
  int                                                         sock;

  for (it = config.begin(); it != config.end(); ++it)
  {
    if (addedPorts.insert(it->port).second == false)
    {
      /*
      **  If port is already bound to a socket, add ServerConfig
      **  as another value to that socket key in ServerConfig.
      */
      for (jt = this->_listeningSockets.begin();
              jt != this->_listeningSockets.end(); ++jt)
      {
        if (it->port == jt->second.front()->port)
          jt->second.push_back(&(*it));
      }
    }
    else
    {
      /*
      **  If port is not bound to a socket yet, set up socket
      **  and add socket as key and ServerConfig as value in _listeningSockets.
      */
      if (!this->_initSocket(sock, it->port))
        return (false);
      this->_listeningSockets[sock].push_back(&(*it));
    }
  }
  return (true);
}

void  Server::_endConnection(int fd, size_t connIndex)
{
  close(fd);
  this->_monitor.remove(connIndex);
  this->_connectionSockets.erase(fd);
  return ;
}

bool  Server::_launchCgi(int socket, ConnectionData & conn,
                          std::size_t connIndex)
{
  CgiResponse * cgi;

  cgi = new CgiResponse(socket, connIndex);
  if (!cgi->initPipes(conn))
  {
    delete cgi;
    return (false);
  }
  //Set non-blocking pipe fds
  if (fcntl(cgi->getWInPipe(), F_SETFL, O_NONBLOCK)
      || fcntl(cgi->getROutPipe(), F_SETFL, O_NONBLOCK))
  {
    std::cerr << "Could not set non-blocking pipe fds" << std::endl;
    close(cgi->getWInPipe());
    close(cgi->getROutPipe());
    delete cgi;
    return (false);
  }
  //Provisional, need to check writen bytes after each call, as it is non-blocking
  write(cgi->getWInPipe(), "Hola Mundo!", 11);
  close(cgi->getWInPipe());
  //Associate read pipe fd with cgi class instance
  this->_cgiPipes.insert(std::pair<int, CgiResponse *>(cgi->getROutPipe(), cgi));
  //Check POLLIN event of read pipe fd with poll()
  this->_monitor.add(cgi->getROutPipe(), POLLIN);
  return (true);
}

/*
**  Calls the appropiate Response's readFile method.
*/

bool  Server::_fillFileResponse(int const fd, int const index)
{
  std::pair<int, std::size_t> sockPair = this->_fileFds[fd];
  ConnectionData &            sockData =
    this->_connectionSockets[sockPair.first];

  if (!sockData.totalBytesRead)
  {
    if (!this->_response.readFile(sockData.filePath, fd, sockData.rsp,
                              sockData.totalBytesRead, sockData.fileSize))
    {
      this->_endConnection(sockPair.first, sockPair.second);
      return (false);
    }
    sockData.rspSize = sockData.rsp.size() - sockData.totalBytesRead +
                        sockData.fileSize;
  }
  else
  {
    if (!this->_response.readFile(fd, sockData.rsp, sockData.totalBytesRead))
    {
      close(fd);
      this->_monitor.remove(index);
      this->_fileFds.erase(fd);
      this->_endConnection(sockPair.first, sockPair.second);
      return (false);
    }
  }
  if (static_cast<long>(sockData.totalBytesRead) == sockData.fileSize)
  {
    close(fd);
    this->_monitor.remove(index);
    this->_fileFds.erase(fd);
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
  res.append("Content-type: text/html\n\n");
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
    conn.rsp = this->_listDir(path, loc->root);
    conn.rspSize = conn.rsp.size();
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
  ConnectionData &  sockData = this->_connectionSockets[socket];

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
  ConnectionData &  sockData = this->_connectionSockets[socket];

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
    this->_fileFds[sockData.fileFd] = std::pair<int,std::size_t>(socket, index);
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
  ConnectionData &  sockData = this->_connectionSockets[socket];

  if (!this->_response.sendFile(socket, sockData.rsp,
      sockData.totalBytesSent))
  {
    if (this->_fileFds.count(sockData.fileFd))
    {
      close(sockData.fileFd);
      this->_fileFds.erase(sockData.fileFd);
      //TODO: delete fileFd from _monitor
    }
    this->_endConnection(socket, index);
  }
  if (sockData.totalBytesSent == sockData.rspSize)
    this->_endConnection(socket, index);
}

/*
**  Receive data from a cgi pipe's read end.
**
**  Need to check Content-length header from cgi response to know when
**  all the data from cgi program has been read.
*/

bool  Server::_receiveCgiData(int rPipe)
{
  int const         connSocket = this->_cgiPipes[rPipe]->getSocket();
  ConnectionData &  connData = this->_connectionSockets[connSocket];
  std::size_t const buffLen = 500;
  char              buff[buffLen + 1];
  int               len;

  std::fill(buff, buff + buffLen + 1, 0);
  /*
  **  TODO: Handle this read in a truly non-blocking way.
  */
  while (1)
  {
    len = read(rPipe, buff, buffLen);
    if (len == 0)
    {
      std::cout << "Pipe connection closed unexpectedly." << std::endl;
      return (false);
    }
    if (len < 0)
      break ;
    connData.rsp.append(buff, len);
    std::fill(buff, buff + buffLen, 0);
  }
  /*
  **  Provisional. rspSize must be obtained by parsing the Content-length
  **  header sent by cgi program
  */
  connData.rspSize = connData.rsp.size();
  // Check again for POLLOUT event of client socket associated to cgi pipe fd
  this->_monitor[this->_cgiPipes[rPipe]->getIndex()].events = POLLOUT;
  return (true);
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
  this->_connectionSockets[socket].req.process(reqData);
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
  int                                   newConn;

  /*
  **  Get configs vector associated to listeningSocket in advance
  **  to avoid repeated searches in case more than one connection
  **  has been received through this port.
  */
  configs = &this->_listeningSockets[listenSocket];
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
    this->_connectionSockets[newConn].portConfigs = configs;
  }
}

/*
**  Handle events from poll fds.
*/

void  Server::_handleEvent(std::size_t index)
{
  int fd;

  fd = this->_monitor[index].fd;
  if (this->_listeningSockets.count(fd))
  {
    // New client/s connected to one of listening sockets
    this->_acceptConn(fd);
  }
  else if (this->_cgiPipes.count(fd))
  {
    // Read pipe from cgi program is ready to read
    if (!this->_receiveCgiData(fd))
      return ; //Handle Error
    this->_monitor.remove(index);
    delete this->_cgiPipes[fd];
    this->_cgiPipes.erase(fd); //remove cgi class instance of read pipe fd
    close(fd); //close pipe read fd
  }
  else if (this->_fileFds.count(fd))
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
              << this->_connectionSockets[fd].req.getPetit("Host")
              << " with path "
              << this->_connectionSockets[fd].req.getPetit("Path")
              << std::endl;
    if (!this->_prepareResponse(fd, index))
      this->_endConnection(fd, index);
  }
  else
  {
    // Connected client socket is ready to write without blocking
    this->_sendData(fd, index);
  }
}

/*
**  Add listening sockets of serverConfigs to connections array
*/

void  Server::_monitorListenSocketEvents(void)
{
  std::map<int, std::vector<ServerConfig const *> >::iterator it;

  for (it = this->_listeningSockets.begin();
          it != this->_listeningSockets.end(); ++it)
  {
    this->_monitor.add(it->first, POLLIN);
  }
  return ;
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

  this->_monitorListenSocketEvents();
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
