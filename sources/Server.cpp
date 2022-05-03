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

bool  Server::_launchCgi(int socket/*, ConnectionData const & conn*/,
                          std::size_t connIndex)
{
  CgiResponse * cgi;

  cgi = new CgiResponse(socket, connIndex);
  if (!cgi->initPipes())
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
**  Provisional, need Request instance to get more info about the request.
**
**  A hash table could be implemented to avoid some file I/O and
**  increase response efficiency.
*/

void  Server::_getResponse(std::string & data, ConnectionData const & conn) const
{
  ServerConfig const *    serv = (*conn.portConfigs)[conn.serverIndex];
  LocationConfig const *  loc = &serv->location[conn.locationIndex];
  std::string             fileName;
  std::ifstream           file;

  fileName = loc->root + '/' + loc->default_file;
  file.open(fileName.c_str());
  if (file.is_open())
    file.close();
  else
    fileName = serv->not_found_page;
  data = Response(fileName).get();
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
  // Match server
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
  ConnectionData *  sockData = &this->_connectionSockets[socket];
  std::size_t       needle;
  std::string       host;
  std::string       uri;

  needle = sockData->dataIn.find("Host:") + 6;
  host = sockData->dataIn.substr(needle,
          sockData->dataIn.find("\n", needle) - needle);
  this->_matchServer(*(sockData->portConfigs),
                        sockData->serverIndex, host);
  needle = sockData->dataIn.find(" ") + 1;
  uri = sockData->dataIn.substr(needle,
          sockData->dataIn.find(" ", needle) - needle);
  this->_matchLocation((*sockData->portConfigs)[sockData->serverIndex]->location,
                        sockData->locationIndex, uri);
}

/*
**  Sends available data to a client.
**
**  Depending on the request, it will send the requested file
**  or execute a cgi process and send its result.
*/

bool  Server::_sendData(int socket, std::size_t index)
{
  ConnectionData *  sockData = &this->_connectionSockets[socket];
  std::string &     dataOut = sockData->dataOut;
  int               sent;
  std::size_t       totalSent;

  /*
  **  This is provisional. Need to parse the request, check the location
  **  configuration properly, and search in the root directory of
  **  the location for the requested or default file
  */
  this->_matchConfig(socket);
  if (dataOut == "")
  {
    if (sockData->dataIn.find(".cgi") != std::string::npos) //provisional
    {
      if (!this->_launchCgi(socket/*, *sockData*/, index))
        return (false);
      /*
      **  Do not check for events from this client socket until we receive data
      **  from cgi program.
      */
      this->_monitor[index].events = 0;
      return (true);
    }
    this->_getResponse(dataOut, *sockData);
  }
  totalSent = 0;
  /*
  **  DETERMINE IF INSTEAD OF A LOOP THAT BLOCKS UNTIL THE ENTIRE DATA IS SENT,
  **  IS MORE EFFICIENT TO SEND AGAIN TO poll AND CHECK AGAIN FOR POLLOUT UNTIL
  **  THE ENTIRE DATA IS SENT.
  */
  while (totalSent != dataOut.length()) // send might not send all dataOut in one call
  {
    sent = send(socket, dataOut.c_str() + totalSent,
                  dataOut.length() - totalSent, 0);
    if (sent < 0)
    {
      std::cout << "Could not send data to client." << std::endl;
      return (false);
    }
    totalSent += sent;
  }
  close(socket);
  this->_monitor.remove(index);
  this->_connectionSockets.erase(socket);
  return (true);
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
  std::string &     dataOut = this->_connectionSockets[connSocket].dataOut;
  std::size_t const buffLen = 500;
  char              buff[buffLen + 1];
  int               len;

  std::fill(buff, buff + buffLen + 1, 0);
  /*
  **  DETERMINE IF INSTEAD OF A LOOP THAT BLOCKS UNTIL THE ENTIRE
  **  DATA IS RECEIVED, IS MORE EFFICIENT TO SEND AGAIN TO poll
  **  AND CHECK AGAIN FOR POLLIN UNTIL THE ENTIRE DATA IS RECEIVED.
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
    dataOut.append(buff, len);
    std::fill(buff, buff + buffLen, 0);
  }
  // Check again for POLLOUT event of client socket associated to cgi pipe fd
  this->_monitor[this->_cgiPipes[rPipe]->getIndex()].events = POLLOUT;
  return (true);
}

/*
**  Receive data from a client socket.
*/

bool  Server::_receiveData(int socket)
{
  std::size_t const buffLen = 500;
  char              buff[buffLen + 1];
  int               len;

  std::fill(buff, buff + buffLen + 1, 0);
  /*
  **  DETERMINE IF INSTEAD OF A LOOP THAT BLOCKS UNTIL THE ENTIRE
  **  DATA IS RECEIVED, IS MORE EFFICIENT TO SEND AGAIN TO poll
  **  AND CHECK AGAIN FOR POLLIN UNTIL THE ENTIRE DATA IS RECEIVED.
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
    this->_connectionSockets[socket].dataIn.append(buff, len);
    std::fill(buff, buff + buffLen, 0);
  }
  return (true);
}

/*
**  Accept client connections from any virtual server listening socket
**  and add them to the connections array that is passed to poll.
*/

bool  Server::_acceptConn(int listenSocket)
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
        return (false);
      }
      break ;
    }
    if (fcntl(newConn, F_SETFL, O_NONBLOCK))
    {
      std::cerr << "Could not set non-blocking data socket" << std::endl;
      close(newConn);
      return (false);
    }
    this->_monitor.add(newConn, POLLIN);
    /*
    **  Add new connection socket as key in _connectionSockets
    **  and configs vector as value.
    */
    this->_connectionSockets[newConn].portConfigs = configs;
  }
  return (true);
}

/*
**  Handle 2 types of events:
**
**  1. A listening socket receives new connections from clients.
**  2. When an accepted connection is ready to receive the data
**      and then send data to it.
*/
bool  Server::_handleEvent(std::size_t index)
{
  int fd;

  fd = this->_monitor[index].fd;
  if (this->_listeningSockets.count(fd))
  {
    // New client/s connected to one of listening sockets
    if (!this->_acceptConn(fd))
      return (false);
  }
  else if (this->_cgiPipes.count(fd))
  { // Read pipe from cgi program is ready to read
    if (!this->_receiveCgiData(fd))
      return (false);
    this->_monitor.remove(index);
    delete this->_cgiPipes[fd];
    this->_cgiPipes.erase(fd); //remove cgi class instance of read pipe fd
    close(fd); //close pipe read fd
  }
  else if (this->_monitor[index].revents & POLLIN)
  {
    // Connected client socket is ready to read without blocking
    if (!this->_receiveData(fd))
      return (false);
    std::cout << "Data received !!!!!\n\n";
    std::cout << this->_connectionSockets[fd].dataIn << std::endl;
    this->_monitor[index].events = POLLOUT;
  }
  else
  {
    // Connected client socket is ready to write without blocking
    if (!this->_sendData(fd, index))
      return (false);
  }
  return (true);
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
**  Additionally, poll is also used to know when client sockets and cgi pipes
**  have data to receive or are ready to send data to them.
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
      if (!this->_handleEvent(i))
        break ;
      // To stop iterating when total events have been handled
      if (++handlingCount == numEvents)
        break ;
    }
    this->_monitor.purge();
  } 
  return (true);
}
