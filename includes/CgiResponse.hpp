#pragma once

#include "webserv.hpp"

class CgiResponse
{
private:

  int         _socket;
  std::size_t _connIndex;
  int         _inPipe[2];
  int         _outPipe[2];

  CgiResponse(void);

  void  _execProgram(void);

public:

  CgiResponse(int socket, std::size_t connIndex);
  ~CgiResponse(void);

  int getSocket(void) const;
  std::size_t getIndex(void) const;

  int getRInPipe(void) const;
  int getWInPipe(void) const;
  int getROutPipe(void) const;
  int getWOutPipe(void) const;

  bool  initPipes(void);

};
