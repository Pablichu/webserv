#include "Server.hpp"

Server::Server(void) : _response(_fdTable, _monitor)
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
      /*
      **  Determine if it makes sense to check for POLLOUT if it is never
      **  going to be used for listening sockets.
      */
      this->_monitor.add(sock, POLLIN /*| POLLOUT*/);
    }
  }
  return (true);
}

/*
**  Order of removals is important. fdTable deletes fileData and cgiData.
**
**  IMPORTANT!!!
**
**  Closing the fd must be done after using it to remove its associated
**  data structures, otherwise that fd could be assigned to another connection,
**  and when removing the data structure, it would be removing one from other
**  connection.
*/

void  Server::_endConnection(int fd, size_t connIndex)
{
  ConnectionData &  connData = this->_fdTable.getConnSock(fd);
  int               associatedFd;

  if (connData.fileData)
  { // Order of removals is important. fdTable deletes fileData.
    associatedFd = connData.fileData->fd;
    this->_monitor.removeByFd(associatedFd);
    this->_fdTable.remove(associatedFd);
    close(associatedFd);
    connData.fileData = 0;
  }
  else if (connData.cgiData)
  { // Order of removals is important. fdTable deletes cgiData.
    associatedFd = connData.cgiData->getROutPipe();
    this->_monitor.removeByFd(associatedFd);
    this->_fdTable.remove(associatedFd);
    close(associatedFd);
    connData.cgiData = 0;
  }
  this->_monitor.removeByIndex(connIndex);
  this->_fdTable.remove(fd);
  close(fd);
  return ;
}

/*
**  Calls the appropiate Response's readFile method.
*/

bool  Server::_fillFileResponse(int const fd, int const index)
{
  FileData &        fileData = this->_fdTable.getFile(fd);
  ConnectionData &  connData = this->_fdTable.getConnSock(fileData.socket);

  if (!connData.totalBytesRead)
  {
    if (!this->_response.fileHandler.readFileFirst(fd, connData))
    { // Maybe try to return 500 error?
      //Possible _endConnection function overload? this->_endConnection(fileData.socket, fd, index);
      return (false);
    }
  }
  else
  { // Maybe try to return 500 error?
    if (!this->_response.fileHandler.readFileNext(fd, connData))
    {
      //Possible _endConnection function overload? this->_endConnection(fileData.socket, fd, index);
      return (false);
    }
  }
  if (static_cast<long>(connData.totalBytesRead) == fileData.fileSize)
  {
    this->_monitor.removeByIndex(index);
	  this->_fdTable.remove(fd);
    close(fd);
    connData.fileData = 0;
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
    connData.serverIndex, utils::extractHost(connData.req.getPetit("Host")));
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

void  Server::_handleClientRead(int socket)
{
  int error = 0;

  if (this->_fdTable.getConnSock(socket).req.isChunked())
	this->_fdTable.getConnSock(socket).req.processChunked();
  else
  	this->_fdTable.getConnSock(socket).req.process();
  
  UrlParser().parse(this->_fdTable.getConnSock(socket).req.getPetit("Path"),
                    this->_fdTable.getConnSock(socket).urlData);
  std::cout << "Data received for "
            << this->_fdTable.getConnSock(socket).req.getPetit("Host")
            << " with path "
            << this->_fdTable.getConnSock(socket).req.getPetit("Path")
            << " | Buffer reached: "
			<< this->_fdTable.getConnSock(socket).req.updateLoop(false)
			<< std::endl;
  int error = 0;
  if (!this->_validRequest(socket, error)
      || !this->_response.process(socket, error))
    this->_response.sendError(socket, error);

	std::cout << std::endl;
	std::map<std::string, std::string>::iterator  it;
	/*for (it = this->_fdTable.getConnSock(socket).req.begin(); it != this->_fdTable.getConnSock(socket).req.end(); it++) {
		std::cout << it->first << " = " << it->second << std::endl;
	}*/
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

  if (!this->_response.sendData(socket, sockData))
    this->_endConnection(socket, index);
}

/*
**  Receive data from a client socket.
*/

bool  Server::_receiveData(int socket)
{
  ConnectionData &	cone = this->_fdTable.getConnSock(socket);
  std::string &     reqData = cone.req.getData();
  int               len;
  //Request			*req = &this->_fdTable.getConnSock(socket).req;

  len = recv(socket, &cone.rspBuff[0], cone.rspBuffCapacity, 0);
  if (len <= 0)
  {
    std::cout << "Client connection closed unexpectedly.";
    if (len == -1)
		std::cout << " recv throught an -1";
	std::cout << std::endl;
	return (false);
  }
  /*
  **  Do not append as string, as it will append the entire buffer size
  **  instead of only the non null elements encountered before the first
  **  null element.
  */
  cone.req.getDataSate() = true;

  reqData.append(&cone.rspBuff[0]);
  cone.req.updateLoop(true);
  std::fill(&cone.rspBuff[0], &cone.rspBuff[len], 0);
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
    this->_monitor.add(newConn, POLLIN | POLLOUT);
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
  else if (fdType == PipeWrite)
  {
    if (!this->_response.cgiHandler.sendBody(fd,
      this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket)))
    { // Maybe return 500 error?
      // End connection, write failed.
      return ;
    }
    if (this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket).totalBytesSent
        == this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket).rspSize)
    {
      /*
      **  Close pipe fd, but do not delete CgiData structure,
      **  only PipeRead type must do it, because it is shared between the two.
      */
      /*
      **  The way of obtaining ConnectionData is provisional. This code will
      **  be moved to another function.
      */
      this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket).rspSize = 0;
      this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket).totalBytesSent = 0;
      this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket).cgiData = 0;
      this->_monitor.removeByIndex(index);
	    this->_fdTable.remove(fd);
      close(fd); //close pipe write fd
    }
  }
  else if (fdType == PipeRead)
  {
    // Read pipe from cgi program is ready to read
    if (!this->_response.cgiHandler.receiveData(fd,
        this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket)))
    { // Maybe try to return 500 error?
      //Possible _endConnection function overload? this->_endConnection(this->_fdTable.getPipe(fd).socket, fd, index);
      return ;
    }
    if (this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket).rspSize
		    == this->_fdTable.getConnSock(
          this->_fdTable.getPipe(fd).socket).totalBytesRead)
    { //All data was received
      //Order of removals and close is important!!!
      this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket).cgiData = 0;
      this->_monitor.removeByIndex(index);
	    this->_fdTable.remove(fd);
      close(fd); //close pipe read fd
      return ;
    }
  }
  else if (fdType == File)
  {
    if (this->_monitor[index].revents & POLLIN)
    {
      // A file fd is ready to read.
      if (!this->_fillFileResponse(fd, index))
        std::cout << "Error reading file fd: " << fd << std::endl;
    }
  }
  else
  {
    if (this->_monitor[index].revents & POLLIN)
    {  
      // Connected client socket is ready to read without blocking
	  if (!this->_receiveData(fd))
  	  {
    	this->_endConnection(fd, index); //TODO: Handle Error
    	return ;
  	  }
    }
    else if (this->_monitor[index].revents & POLLOUT)
    {
	  if (this->_fdTable.getConnSock(fd).req.getDataSate() == true)
	  {
		std::cout << "PROCESAMOS" << std::endl;
      	this->_handleClientRead(fd);
	  }
	  // Connected client socket is ready to write without blocking
      if (this->_fdTable.getConnSock(fd).rspBuffSize)    
        this->_sendData(fd, index);
      if (this->_fdTable.getConnSock(fd).totalBytesSent
          == this->_fdTable.getConnSock(fd).rspSize
          && this->_fdTable.getConnSock(fd).totalBytesSent)
        this->_endConnection(fd, index);
    }
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
      if (this->_monitor[i].revents == 0 || this->_monitor[i].fd == -1)
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
