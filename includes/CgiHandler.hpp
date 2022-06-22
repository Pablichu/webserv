#pragma once

#include "webserv.hpp"

struct  CgiData;
struct  ConnectionData;

class CgiHandler
{
private:

  void  _execProgram(CgiData const & cgiData, ConnectionData & connData);

public:

  CgiHandler(void);
  ~CgiHandler(void);

  bool	receiveData(int rPipe, ConnectionData & connData);
  bool  initPipes(CgiData & cgiData, ConnectionData & connData);

};
