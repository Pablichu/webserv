#include "Server.hpp"

int const Server::maxRequests = 1024;

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
	if (listen(sock, Server::maxRequests))
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
      this->_eventHandler.connectionRead(fd);
    }
    if (this->_monitor[index].revents & POLLOUT)
    {// Connected client socket is ready to write without blocking
      this->_eventHandler.connectionWrite(fd);
    }
  }
}

bool  Server::_checkActive(int const fd, ConnectionData & connData)
{
  double        timeSince;
  time_t const  currentTime = time(NULL);

  if (connData.lastSend)
  {
    timeSince = difftime(currentTime, connData.lastSend);
    if (timeSince >= ConnectionData::SendTimeout)
    {
      this->_connHandler.end(fd);
      return (true);
    }
  }
  else
  {
    timeSince = difftime(currentTime, connData.lastRead);
    if (timeSince >= ConnectionData::ReadTimeout)
    {
      this->_connHandler.end(fd);
      return (true);
    }
  }
  return (false);
}

bool  Server::_checkIdle(int const fd, time_t const lastActive)
{
  double        timeIdle;
  time_t const  currentTime = time(NULL);

  timeIdle = difftime(currentTime, lastActive);
  if (timeIdle >= ConnectionData::keepAliveTimeout)
  {
    this->_connHandler.end(fd);
    return (true);
  }
  return (false);
}

bool  Server::_checkTimeout(int const fd)
{
  FdType            fdType;
  /*
  **  Using a pointer to avoid creating a copy of ConnectionData,
  **  which is a big structure.
  */
  ConnectionData *  connData;

  fdType = this->_fdTable.getType(fd);
  if (fdType != ConnSock) // Only client connection sockets have timeout
    return (false);
  connData = &this->_fdTable.getConnSock(fd);
  if (connData->status != Idle) // Active
    return (this->_checkActive(fd, *connData));
  else // Idle
    return (this->_checkIdle(fd, connData->lastActive));
}

//Global var for sigint
bool	G_SIGOUT;

void	signal_handler(int signal)
{
	std::cout << std::endl << "Signal " << signal << " detected. Stopping all process..." << std::endl;
	G_SIGOUT = true;
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

  G_SIGOUT = false;
  signal(SIGINT, signal_handler);
  while (!G_SIGOUT)
  {
    /*
    **  Store monitor's biggestActiveFd before calling poll, so that
    **  the loop ends before empty pollfds at the back and new pollfds
    **  with a bigger fd created during the current iteration are reached.
    */
    biggestActiveFd = static_cast<std::size_t>(
                        this->_monitor.biggestActiveFd());
    errno = 0;
    numEvents = poll(this->_monitor.getFds(), biggestActiveFd + 1,
                      static_cast<int>(ConnectionData::keepAliveTimeout / 2)
                      * 1000);
    if (numEvents < 0)
    {
      if (errno == EINTR)
        return (true);
      std::cout << "poll() error." << std::endl;
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
    this->_monitor.adjustSize();
  }
  return (true);
}
