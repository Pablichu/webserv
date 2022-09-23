#pragma once

#include <iostream>

#include "Config.hpp"
#include "Data.hpp"

struct  ServerConfig;

enum		FdType
{
	None,
	ListenSock,
	ConnSock,
	File,
	PipeRead,
  PipeWrite
};

class FdTable
{
private:

  std::vector< std::pair<	FdType, uintptr_t > > _table;

  uintptr_t _serializeListenSock(std::vector< ServerConfig const * > *  ptr);
  uintptr_t _serializeFile(FileData *  ptr);
  uintptr_t _serializeConnSock(ConnectionData *  ptr);
  uintptr_t _serializePipe(CgiData *  ptr);

  std::vector< ServerConfig const * > * _deserializeListenSock(uintptr_t raw);
  ConnectionData *						_deserializeConnSock(uintptr_t raw);
  FileData *							_deserializeFile(uintptr_t raw);
  CgiData *								_deserializePipe(uintptr_t raw);

  bool	_littleAddChecker(int const fd);
public:

  FdTable(void);
  ~FdTable(void);

  std::size_t size(void) const;

  FdType  getType(int const fd) const;
  std::vector<ServerConfig const *> &	getListenSockData(int const fd);
  ConnectionData &						getConnSock(int const fd);
  FileData &		getFile(int const fd);
  CgiData &								getPipe(int const fd);

  bool  add(int const fd, std::vector< ServerConfig const * > * data);
  bool  add(int const fd, FileData * data);
  bool  add(int const fd, ConnectionData * data);
  bool  add(int const fd, CgiData * data, bool const read);
  void  remove(int const fd);

};
