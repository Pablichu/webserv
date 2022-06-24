#pragma once

#include "Config.hpp"

struct  ServerConfig;

enum		FdType
{
	None,
	ListenSock,
	ConnSock,
	File,
	Pipe
};

class FdTable
{
private:

  std::vector< std::pair<	FdType, uintptr_t > > _table;

  uintptr_t _serializeListenSock(std::vector< ServerConfig const * > *  ptr);
  std::vector< ServerConfig const * > * _deserializeListenSock(uintptr_t raw);

public:

  FdTable(void);
  ~FdTable(void);

  std::size_t size(void) const;
  FdType  getType(int const fd) const;
  std::vector<ServerConfig const *> & getListenSockData(int const fd);

  bool  add(int const fd, std::vector< ServerConfig const * > * data);
  void  remove(int const fd);

};
