#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>

#include "Monitor.hpp"
#include "FdTable.hpp"

class ConnectionHandler
{

public:

  ConnectionHandler(Monitor & monitor, FdTable & fdTable);
  ~ConnectionHandler(void);

  void	accept(int const listenSocket);
  void  end(int const fd, std::size_t const index);
  bool  receive(int const fd);
  void  send(int const fd, std::size_t const index);

private:

  Monitor & _monitor;
  FdTable & _fdTable;

  ConnectionHandler(void);

};
