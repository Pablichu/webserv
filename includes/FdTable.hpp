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

  std::vector< std::pair<	FdType, void * > > _table;

  void *  _serializeListenSock(std::vector< ServerConfig const * > *  ptr);
  void *  _serializeFile(FileData *  ptr);
  void *  _serializeConnSock(ConnectionData *  ptr);
  void *  _serializePipe(CgiData *  ptr);

  std::vector< ServerConfig const * > * _deserializeListenSock(void * raw);
  ConnectionData *						_deserializeConnSock(void * raw);
  FileData *							_deserializeFile(void * raw);
  CgiData *								_deserializePipe(void * raw);

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
