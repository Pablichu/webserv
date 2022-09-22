#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>

#include "Monitor.hpp"
#include "FdTable.hpp"

class ConnectionHandler
{

public:

  ConnectionHandler(Monitor & monitor, FdTable & fdTable);
  ~ConnectionHandler(void);

  void	accept(int const listenSocket);
  void  end(int const fd);
  bool  receive(int const fd);
  void  send(int const fd);

private:

  Monitor & _monitor;
  FdTable & _fdTable;

  ConnectionHandler(void);

};
