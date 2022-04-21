#include "Server.hpp"

Server::Server(void) : _connLen(0), _connCap(100)
{
  this->_connections = new struct pollfd [this->_connCap];
  return ;
}

Server::~Server(void)
{
  std::size_t i;

  if (this->_connLen)
  {
    for (i = 0; i < this->_connLen; ++i)
    {
      close(this->_connections[i].fd);
    }    
  }
  delete[] this->_connections;
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
**  Obtain one listening socket per ServerConfig.
**
**  This function should be called for each listening socket.
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
      this->_connLen += 1;
    }
  }
  return (true);
}

void  Server::_removeConnection(std::size_t index)
{
  std::size_t i;

  for (i = index; i < (this->_connLen - 1); ++i)
    this->_connections[i] = this->_connections[i + 1];
  this->_connLen -= 1;
  return ;
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

bool  Server::_sendData(int socket)
{
  ConnectionData *  sockData = &this->_connectionSockets[socket];
  std::string &     dataOut = sockData->dataOut;
  int               sent;
  std::size_t       totalSent;

  /*
  **  This is provisional. Need to parse the request, check the location
  **  configuration properly, and search in the root directory of
  **  the location for the file
  */
  this->_matchConfig(socket);
  this->_getResponse(dataOut, *sockData);
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
  return (true);
}

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

void  Server::_increaseConnCap(void)
{
  struct pollfd * aux;

  aux = new struct pollfd [this->_connCap * 2];
  std::copy(this->_connections, this->_connections + this->_connLen, aux);
  delete[] this->_connections;
  this->_connections = aux;
  this->_connCap *= 2;
  return ;
}

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
    if (this->_connLen == this->_connCap)
      this->_increaseConnCap();
    this->_connections[this->_connLen].fd = newConn;
    this->_connections[this->_connLen].events = POLLIN;
    this->_connLen += 1;
    /*
    **  Add new connection socket as key in _connectionSockets
    **  and configs vector as value.
    */
    this->_connectionSockets[newConn].portConfigs = configs;
  }
  return (true);
}

/* Handle 2 types of events:
**
**  1. A listening socket receives new connections from clients.
**  2. When an accepted connection is ready to receive the data
**      and then send data to it.
*/
bool  Server::_handleEvent(std::size_t index)
{
  int   socket;
  bool  ok;

  ok = true;
  socket = this->_connections[index].fd;
  if (this->_listeningSockets.count(socket))
  {
    // New client/s connected to one of listening sockets
    if (!this->_acceptConn(socket))
      return (false);
  }
  else if (this->_connections[index].revents & POLLIN)
  {
    // Connected client socket is ready to read without blocking
    if (!this->_receiveData(socket))
      return (false);
    std::cout << "Data received !!!!!\n\n";
    std::cout << this->_connectionSockets[socket].dataIn << std::endl;
    this->_connections[index].events = POLLOUT;
  }
  else
  {
    // Connected client socket is ready to write without blocking
    if (!this->_sendData(socket))
      ok = false;
    close(socket);
    this->_removeConnection(index);
    this->_connectionSockets.erase(socket);
  }
  return (ok);
}

void  Server::_monitorListenSocketEvents(void)
{
  std::map<int, std::vector<ServerConfig const *> >::iterator it;
  std::size_t                                           i;

  for (it = this->_listeningSockets.begin(), i = 0;
          it != this->_listeningSockets.end(); ++it, ++i)
  {
    this->_connections[i].fd = it->first;
    this->_connections[i].events = POLLIN;
  }  
  return ;
}

bool  Server::start(void)
{
  std::size_t loopConnLen;
  int         numEvents;
  int         handlingCount;

  this->_monitorListenSocketEvents();
  while (true)
  {
    loopConnLen = this->_connLen; //Update number of monitored socket for next poll call
    numEvents = poll(this->_connections, loopConnLen, -1); // TIMEOUT -1 blocks until event is received
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
    for (std::size_t i = 0; i < loopConnLen; ++i) // INEFFICIENT!! USE kqueue INSTEAD of poll
    {
      if (this->_connections[i].revents == 0)
        continue;
      if (!this->_handleEvent(i))
        break ;
      if (++handlingCount == numEvents) // To stop iterating when total events have been handled
        break ;
    }    
  } 
  return (true);
}
