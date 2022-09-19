#pragma once

#include "Monitor.hpp"
#include "FdTable.hpp"
#include "ConnectionHandler.hpp"
#include "ConfigMatcher.hpp"
#include "Response.hpp"

class EventHandler
{

public:

  EventHandler(Monitor & monitor, FdTable & fdTable,
                ConnectionHandler & connHandler);
  ~EventHandler(void);

  void	connectionAccept(int const listenSocket);
  void  connectionRead(int const fd);
  void  connectionWrite(int const fd);
  void  pipeWrite(int const fd);
  void  pipeRead(int const fd);
  void  fileRead(int const fd);
  void  fileWrite(int const fd);

private:

  Monitor &           _monitor;
  FdTable &           _fdTable;
  ConnectionHandler & _connHandler;
  ConfigMatcher       _configMatcher;
  Response            _response;

  EventHandler(void);

  void  _processConnectionRead(int const fd);
  bool	_validRequest(int const fd, int & error);

};
