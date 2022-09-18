#include "Server.hpp"

Server::Server(void) : _monitor(_fdTable),
                        _connHandler(_monitor, _fdTable),
                        _eventHandler(_monitor, _fdTable, _connHandler)
{}

Server::~Server(void)
{}

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
      this->_monitor.add(sock, POLLIN, false);
    }
  }
  return (true);
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
  {// New client/s connected to one of listening sockets
    this->_eventHandler.connectionAccept(fd);
  }
  else if (fdType == PipeWrite)
  {// Write pipe from cgi process is ready to write
    this->_eventHandler.pipeWrite(fd);
  }
  else if (fdType == PipeRead)
  {// Read pipe from cgi process has data to read
    this->_eventHandler.pipeRead(fd);
  }
  else if (fdType == File)
  {
    if (this->_monitor[index].revents & POLLIN)
    {// A file fd is ready to read.
      this->_eventHandler.fileRead(fd);
    }
    else // POLLOUT
    {// A file fd is ready to write.
      this->_eventHandler.fileWrite(fd);
    }
  }
  else
  {
    if (this->_monitor[index].revents & POLLIN)
    {// Connected client socket is ready to read without blocking
      this->_eventHandler.connectionRead(fd, index);
    }
    else if (this->_monitor[index].revents & POLLOUT)
    {// Connected client socket is ready to write without blocking
      this->_eventHandler.connectionWrite(fd, index);
    }
  }
}

bool  Server::_checkTimeout(int const fd)
{
  FdType            fdType;
  /*
  **  Using a pointer to avoid creating a copy of ConnectionData,
  **  which is a big structure.
  */
  ConnectionData *  connData;
  double            timeIdle;
  time_t const      currentTime = time(NULL);

  fdType = this->_fdTable.getType(fd);
  if (fdType != ConnSock) // Only client connection sockets have timeout
    return (false);
  connData = &this->_fdTable.getConnSock(fd);
  if (connData->status != Idle)
    return (false);
  timeIdle = difftime(currentTime, connData->lastActive);
  if (timeIdle >= ConnectionData::timeout)
  {
    this->_connHandler.end(fd);
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
  std::size_t biggestActiveFd;

  while (true)
  {
    /*
    **  Store monitor's biggestActiveFd before calling poll, so that
    **  the loop ends before empty pollfds at the back and new pollfds
    **  with a bigger fd created during the current iteration are reached.
    */
    biggestActiveFd = static_cast<std::size_t>(
                        this->_monitor.biggestActiveFd());
    numEvents = poll(this->_monitor.getFds(), biggestActiveFd + 1,
                      static_cast<int>(ConnectionData::timeout / 2) * 1000);
    if (numEvents < 0)
    {
      std::cerr << "poll() error" << std::endl;
      return (false);
    }
    for (std::size_t i = 0; i <= biggestActiveFd; ++i)
    { // INEFFICIENT!! Not using kqueue for compatibility issues
      if (this->_monitor[i].fd == -1
          || this->_checkTimeout(this->_monitor[i].fd)
          || this->_monitor[i].revents == 0)
        continue;
      this->_handleEvent(i);
    }
  }
  return (true);
}
