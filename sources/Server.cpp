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
      this->_monitor.add(sock, POLLIN);
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
  }
  else if (connData.cgiData)
  { // Order of removals is important. fdTable deletes cgiData.
    associatedFd = connData.cgiData->getROutPipe();
    connData.cgiData->closePipes();
    this->_monitor.removeByFd(associatedFd);
    this->_fdTable.remove(associatedFd);
  }
  else if (connData.dirListNeedle)
    closedir(connData.dirListNeedle);
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
  ConnectionData &  connData = this->_fdTable.getConnSock(fileData.socket.fd);
  InputOutput &     io = connData.io;

  if (io.isFirstRead())
  { // First read
    if (!this->_response.fileHandler.readFileFirst(fd, connData))
      return (false); // Read failed
  }
  else
  {
    if (!this->_response.fileHandler.readFileNext(fd, connData))
      return (false); // Read failed
  }
  if (!(fileData.socket.events & POLLOUT))
    fileData.socket.events = POLLIN | POLLOUT;
  if (io.finishedRead())
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
  std::string       path = connData.urlData.find("PATH")->second;

  this->_matchServer(
    *(connData.portConfigs),
    connData.serverIndex, utils::extractHost(connData.req.getPetit("HOST")));
  if (connData.urlData.count("FILENAME"))
    path.erase(path.rfind("/"));
  if (!this->_matchLocation(
      connData.getServer()->location,
      connData.locationIndex, path))
    return (false);
  return (true);
}

void  Server::_handlePipeRead(int const fd, std::size_t const index)
{
  pollfd &          socket = this->_fdTable.getPipe(fd).socket;
  ConnectionData &  connData = this->_fdTable.getConnSock(socket.fd);
  int               error = 0;
  int               exitStatus;

  exitStatus = this->_response.cgiHandler.getExitStatus(connData.cgiData->pID);
  if (exitStatus < 0
      || !this->_response.cgiHandler.receiveData(fd, connData))
  {
    std::cout << "Pipe read failed." << std::endl;
    if (!exitStatus)
      this->_response.cgiHandler.terminateProcess(connData.cgiData->pID);
    // Build Internal Server Error
    this->_response.sendError(connData.cgiData->socket, 500);
    connData.cgiData->closeROutPipe();
    connData.cgiData = 0;
    this->_monitor.removeByIndex(index);
    this->_fdTable.remove(fd);
    return ;
  }
  if (!(socket.events & POLLOUT) && connData.io.getBufferSize())
    socket.events = POLLIN | POLLOUT;
  if (connData.io.finishedRead())
  { //All data was received
    if (!exitStatus)
      this->_response.cgiHandler.terminateProcess(connData.cgiData->pID);
    connData.cgiData->closeROutPipe();
    //Order of removals and close is important!!!
    connData.cgiData = 0;
    this->_monitor.removeByIndex(index);
    this->_fdTable.remove(fd);
    if (connData.io.getPayloadSize() == 0)
    {
      if (!this->_response.process(socket, error))
        this->_response.sendError(socket, error);
    }
  }
}

bool  Server::_validRequest(int socket, int & error)
{
  ConnectionData &  connData = this->_fdTable.getConnSock(socket);

  if (!this->_matchConfig(socket))
  { //Request path did not match any server's location uri
    error = 404; // Not Found
    return (false);
  }
  if (!connData.getLocation()->methods.count(connData.req.getPetit("METHOD")))
  { //Request method is not allowed in target location
    error = 405; // Method Not Allowed
  }
  //else if (!validFileExtension)
    //error = 415; // Unsupported Media Type
  //else if (!validStandardHeaders)
    //error = 400; // Bad Request
  else if (strcmp(connData.req.getPetit("PROTOCOL").c_str(), "HTTP/1.1"))
  {
	  std::cout << connData.req.getPetit("PROTOCOL") << " | " << connData.req.getPetit("PROTOCOL").size() << std::endl;
    error = 505; // HTTP Version Not Supported
  }
  if (error)
    return (false);
  return (true);
}

void  Server::_handleClientRead(pollfd & socket)
{
  ConnectionData & connData = this->_fdTable.getConnSock(socket.fd);

  if (connData.req.processWhat())
  {
    this->_response.sendError(socket, 413);
	return ;
  }
  if (connData.req.getDataSate() != done)
	return ;
  connData.io.clear();
  UrlParser().parse(connData.req.getPetit("PATH"),
                    connData.urlData);
  std::cout << "Data received for "
            << connData.req.getPetit("HOST")
            << " with path "
            << connData.req.getPetit("PATH")
            << " | Buffer reached: "
			<< connData.req.updateLoop(false)
			<< std::endl;

  int error = 0;
  if (!this->_validRequest(socket.fd, error)
      || !this->_response.process(socket, error))
    this->_response.sendError(socket, error);
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
  InputOutput &     io = cone.io;
  std::string &     reqData = cone.req.getData();
  int               len;

  if (io.getAvailableBufferSize() == 0)
    return (true);
  len = recv(socket, io.inputBuffer(), io.getAvailableBufferSize(), 0);
  if (len <= 0)
  {
    std::cout << "Client connection closed unexpectedly.";
    if (len == -1)
		std::cout << " recv throught an -1";
	std::cout << std::endl;
	return (false);
  }
  io.addBytesRead(len);
  /*
  **  Do not append as string, as it will append the entire buffer size
  **  instead of only the non null elements encountered before the first
  **  null element.
  */
  reqData.append(io.outputBuffer(), len);
  cone.req.dataAvailible() = true;
  io.addBytesSent(len);
  cone.req.updateLoop(true);
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
  else if (fdType == PipeWrite)
  {
    if (!this->_response.cgiHandler.sendBody(fd,
      this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket.fd)))
    {
      std::cout << "sendBody to PipeWrite failed." << std::endl;
      // Build Internal Server Error
      this->_response.sendError(this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket.fd).cgiData->socket, 500);
      // Remove cgiData from ConnectionData, closing both pipes.
      this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket.fd).cgiData->closePipes();
      this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket.fd).cgiData = 0;
      this->_monitor.removeByIndex(index);
      this->_fdTable.remove(fd);
      return ;
    }
    if (this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket.fd).io.finishedSend())
    {
      /*
      **  Close pipe fd, but do not delete CgiData structure,
      **  only PipeRead type must do it, because it is shared between the two.
      */
      /*
      **  The way of obtaining ConnectionData is provisional. This code will
      **  be moved to another function.
      */
      this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket.fd).io.clear();
      this->_monitor.removeByIndex(index);
	    this->_fdTable.remove(fd);
      //CgiData has not been deleted yet, because the Read Pipe is still open
      this->_fdTable.getConnSock(this->_fdTable.getPipe(fd).socket.fd).cgiData->closeWInPipe();
    }
  }
  else if (fdType == PipeRead)
  {
    // Read pipe from cgi program is ready to read
    this->_handlePipeRead(fd, index);
  }
  else if (fdType == File)
  {
    if (this->_monitor[index].revents & POLLIN)
    {
      // A file fd is ready to read.
      if (!this->_fillFileResponse(fd, index))
      {
        std::cout << "Error reading file fd: " << fd << std::endl;
        // Build Internal Server Error
        this->_response.sendError(this->_fdTable.getFile(fd).socket, 500);
        // Delete fileData
        this->_fdTable.getConnSock(this->_fdTable.getFile(fd).socket.fd).fileData = 0;
        this->_monitor.removeByIndex(index);
        this->_fdTable.remove(fd);
        // File fd already closed by fillFileResponse on failure
      }
    }
    else // POLLOUT
    {// A file fd is ready to write.
      /*
      **  The way of obtaining ConnectionData is provisional. This code will
      **  be moved to another function.
      */
      if (!this->_response.fileHandler.writeFile(fd,
          this->_fdTable.getConnSock(this->_fdTable.getFile(fd).socket.fd)))
      {// Order of statements is important!!
        // Build Internal Server Error
        this->_response.sendError(this->_fdTable.getFile(fd).socket, 500);
        // Delete fileData
        this->_fdTable.getConnSock(this->_fdTable.getFile(fd).socket.fd).fileData = 0;
        this->_monitor.removeByIndex(index);
        this->_fdTable.remove(fd);
        // File fd already closed by writeFile on failure
        return ;
      }
      if (this->_fdTable.getConnSock(this->_fdTable.getFile(fd).socket.fd).io.finishedSend())
      {
        //Order of statements is important!!
        this->_fdTable.getConnSock(this->_fdTable.getFile(fd).socket.fd).io.clear();
        this->_response.buildUploaded(
          this->_fdTable.getFile(fd).socket,
          this->_fdTable.getConnSock(this->_fdTable.getFile(fd).socket.fd),
          this->_fdTable.getConnSock(
            this->_fdTable.getFile(fd).socket.fd).req.getPetit("PATH"));
        this->_fdTable.getConnSock(this->_fdTable.getFile(fd).socket.fd).fileData = 0;
        this->_monitor.removeByIndex(index);
        this->_fdTable.remove(fd);
        close(fd);
      }
    }
  }
  else
  {
    if (this->_monitor[index].revents & POLLIN)
    {
      if (this->_fdTable.getConnSock(fd).status == Idle)
        this->_fdTable.getConnSock(fd).status = Active;
      // Connected client socket is ready to read without blocking
      if (!this->_receiveData(fd))
  	  {
        this->_endConnection(fd, index);
        return ;
  	  }
      if (this->_fdTable.getConnSock(fd).req.getDataSate() != done
          && this->_fdTable.getConnSock(fd).req.dataAvailible())
      	this->_handleClientRead(this->_monitor[index]);
    }
    else if (this->_monitor[index].revents & POLLOUT)
    {
      // Connected client socket is ready to write without blocking
      if (this->_fdTable.getConnSock(fd).io.getBufferSize())
        this->_sendData(fd, index);
      if (this->_fdTable.getConnSock(fd).dirListNeedle)
      { // build next directory listing chunk
        this->_response.buildDirList(this->_monitor[index],
                                      this->_fdTable.getConnSock(fd));
      }
      //This check is relevant, because the entire buffer might not be sent.
      if(this->_fdTable.getConnSock(fd).io.getBufferSize() == 0)
        this->_monitor[index].events = POLLIN;
      if (this->_fdTable.getConnSock(fd).io.finishedSend() && 
          this->_fdTable.getConnSock(fd).io.getPayloadSize() != 0)
      { // Request handling finished succesfully
        if (this->_fdTable.getConnSock(fd).req.getPetit("CONNECTION") != "close"
            && this->_fdTable.getConnSock(fd).handledRequests
                < ConnectionData::max - 1)
          this->_fdTable.getConnSock(fd).setIdle();
        else
          this->_endConnection(fd, index);
      }
    }
  }
}

bool  Server::_checkTimeout(int const fd, std::size_t const index)
{
  FdType            fdType;
  /*
  **  Using a pointer to avoid creating a copy of ConnectionData,
  **  which is a big structure.
  */
  ConnectionData *  connData;
  double            timeIdle;
  time_t const      currentTime = time(NULL);

  if (fd == -1) // Removed fd during this poll iteration
    return (false);
  fdType = this->_fdTable.getType(fd);
  if (fdType != ConnSock) // Only client connection sockets have timeout
    return (false);
  connData = &this->_fdTable.getConnSock(fd);
  if (connData->status != Idle)
    return (false);
  timeIdle = difftime(currentTime, connData->lastActive);
  if (timeIdle >= ConnectionData::timeout)
  {
    this->_endConnection(fd, index);
    return (true);
  }
  return (false);
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
  std::size_t monitorLen;

  while (true)
  {
    //Store monitor length before calling poll. monitor can grow during loop.
    monitorLen = this->_monitor.len();
    numEvents = poll(this->_monitor.getFds(), monitorLen,
                      static_cast<int>(ConnectionData::timeout / 2) * 1000);
    if (numEvents < 0)
    {
      std::cerr << "poll() error" << std::endl;
      return (false);
    }
    for (std::size_t i = 0; i < monitorLen; ++i)
    { // INEFFICIENT!! Not using kqueue for compatibility issues
      if (this->_checkTimeout(this->_monitor[i].fd, i)
          || this->_monitor[i].revents == 0
          || this->_monitor[i].fd == -1)
        continue;
      this->_handleEvent(i);
    }
    this->_monitor.purge();
  }
  return (true);
}
