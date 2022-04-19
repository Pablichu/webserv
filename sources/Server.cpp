#include "Server.hpp"

Server::Server(void) : _addrlen(sizeof(_address)), _connCap(100)
{
  // Call Config class getConfig to fill _sockets
  this->_sockets.push_back(0);
  this->_connections = new struct pollfd [this->_connCap];
  this->_connLen = this->_sockets.size();
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

/*
**  Obtain one listening socket per ServerConfig.
**
**  This function should be called for each listening socket.
*/

bool  Server::prepare(void)
{
  int & sock = this->_sockets[0]; // PENDING loop implementation
  int verdad = 1;

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
	this->_address.sin_family = AF_INET;
	this->_address.sin_addr.s_addr = INADDR_ANY;
	this->_address.sin_port = htons( PORT );
  for (int i = 0; i < (int)sizeof(this->_address.sin_zero); i++)
		this->_address.sin_zero[i] = 0;

  //	Binding socket
	if (bind(sock, (struct sockaddr *)&(this->_address),
        sizeof(this->_address)))
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

void  Server::_removeConnection(std::size_t index)
{
  std::size_t i;

  for (i = index; i < (this->_connLen - 1); ++i)
    this->_connections[i] = this->_connections[i + 1];
  this->_connLen -= 1;
  return ;
}

bool  Server::_sendData(int socket, std::string & data)
{
  Response    res("hola.html");
  std::string resData;
  int         sent;
  std::size_t totalSent;

  (void)data; // Not reading data for now...
  resData = res.get();
  totalSent = 0;
  while (totalSent != resData.length()) // send might not send all resData in one call
  {
    sent = send(socket, resData.c_str() + totalSent, resData.length() - totalSent, 0);
    if (sent < 0)
    {
      std::cout << "Could not send data to client." << std::endl;
      return (false);
    }
    totalSent += sent;
  }
  return (true);
}

bool  Server::_receiveData(int socket, std::string & data)
{
  std::size_t const buffLen = 500;
  char              buff[buffLen + 1];
  int               len;

  std::fill(buff, buff + buffLen + 1, 0);
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
    data.append(buff, len);
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
  int newConn;

  while (true)
  {
    newConn = accept(listenSocket, (struct sockaddr *)&this->_address,
                      (socklen_t *)&this->_addrlen);
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
  int         socket;
  std::string data;
  bool        ok;

  ok = true;
  socket = this->_connections[index].fd;
  if (std::find(this->_sockets.begin(), this->_sockets.end(),
        socket) != this->_sockets.end())
  {
    // New client/s connected to one of listening sockets
    if (!this->_acceptConn(socket))
      return (false);
  }
  else
  {
    // Connected client socket is ready to read
    if (!this->_receiveData(socket, data))
      return (false);
    std::cout << "Data received !!!!!\n\n" << data << std::endl;
    if (!this->_sendData(socket, data))
      ok = false;
    close(socket);
    this->_removeConnection(index);
  }
  return (ok);
}

void  Server::_monitorListenSocketEvents(void)
{
  std::size_t i;

  for (i = 0; i < this->_connLen; i++)
  {
    this->_connections[i].fd = this->_sockets[i];
    this->_connections[i].events = POLLIN; //Maybe also POLLOUT
  }  
  return ;
}

bool  Server::start(void)
{
  std::size_t loopConnLen;
  int         res;
  int         handlingCount;

  this->_monitorListenSocketEvents();
  while (true)
  {
    loopConnLen = this->_connLen; //Update number of monitored socket for next poll call
    res = poll(this->_connections, loopConnLen, -1); // TIMEOUT -1 blocks until event is received
    if (res < 0)
    {
      std::cerr << "poll() error" << std::endl;
      return (false);
    }
    /*if (res == 0)
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
      if (++handlingCount == res) // To stop iterating when total events have been handled
        break ;
    }    
  } 
  return (true);
}
