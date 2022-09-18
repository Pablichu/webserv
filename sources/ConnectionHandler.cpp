#include "ConnectionHandler.hpp"

ConnectionHandler::ConnectionHandler(Monitor & monitor, FdTable & fdTable)
                                      : _monitor(monitor), _fdTable(fdTable)
{}

ConnectionHandler::~ConnectionHandler(void)
{}

void  ConnectionHandler::accept(int const listenSocket)
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
    /*
    **  Added :: before accept to call the C standard library accept,
    **  and not ConnectionHandler's.
    */
    newConn = ::accept(listenSocket, (struct sockaddr *)&address, &addrLen);
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
    this->_monitor.add(newConn, POLLIN, true);
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
**  Order of removals is important. fdTable deletes fileData and cgiData.
**
**  IMPORTANT!!!
**
**  Closing the fd must be done after using it to remove its associated
**  data structures, otherwise that fd could be assigned to another connection,
**  and when removing the data structure, it would be removing one from other
**  connection.
*/

void  ConnectionHandler::end(int const fd)
{
  ConnectionData &  connData = this->_fdTable.getConnSock(fd);
  int               associatedFd;
  int               auxAssociatedFd;

  if (connData.fileData)
  { // Order of removals is important. fdTable deletes fileData.
    associatedFd = connData.fileData->fd;
    this->_monitor.remove(associatedFd);
    this->_fdTable.remove(associatedFd);
    close(associatedFd);
  }
  else if (connData.cgiData)
  { // Order of removals is important. fdTable deletes cgiData.
    associatedFd = connData.cgiData->getROutPipe();
    auxAssociatedFd = connData.cgiData->getWInPipe();
    connData.cgiData->closePipes();
    if (auxAssociatedFd != -1)
      this->_monitor.remove(auxAssociatedFd);
    this->_monitor.remove(associatedFd);
    this->_fdTable.remove(associatedFd);
  }
  else if (connData.dirListNeedle)
    closedir(connData.dirListNeedle);
  this->_monitor.remove(fd);
  this->_fdTable.remove(fd);
  close(fd);
  return ;
}

bool  ConnectionHandler::receive(int const fd)
{
  ConnectionData &	cone = this->_fdTable.getConnSock(fd);
  InputOutput &     io = cone.io;
  std::string &     reqData = cone.req.getData();
  int               len;

  if (io.getAvailableBufferSize() == 0)
    return (true);
  len = recv(fd, io.inputBuffer(), io.getAvailableBufferSize(), 0);
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
**  It may be called more than once if payload size
**  is bigger than buffer size.
*/

void  ConnectionHandler::send(int const fd)
{
  std::size_t       bytesSent;
  ConnectionData &  connData = this->_fdTable.getConnSock(fd);
  InputOutput &     io = connData.io;

  /*
  **  Using :: before send to call the C standard library send,
  **  and not ConnectionHandler's
  */
  bytesSent = ::send(fd, io.outputBuffer(), io.getBufferSize(), 0);
	if (bytesSent <= 0)
	{
		std::cout << "Could not send data to client." << std::endl;
		this->end(fd);
    return ;
	}
	io.addBytesSent(bytesSent);
	return ;
}
