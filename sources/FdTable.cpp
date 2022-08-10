#include "FdTable.hpp"

FdTable::FdTable(void)
{
  return ;
}

FdTable::~FdTable(void)
{
	std::vector< std::pair<	FdType, uintptr_t > >::iterator it;

	for (it = this->_table.begin(); it != this->_table.end(); ++it)
	{
    	if (it->first == ListenSock)
			delete this->_deserializeListenSock(it->second);
		if (it->first == ConnSock)
			delete this->_deserializeConnSock(it->second);
		if (it->first == File)
			delete this->_deserializeFile(it->second);
		if (it->first == Pipe)
			delete this->_deserializePipe(it->second);
    	it->first = None;
    	it->second = 0;
	}
	return ;
}

std::size_t FdTable::size(void) const
{
  return (this->_table.size());
}

/*
**  WARNING! This method causes undefined behaviour if fd >= _table size
*/

FdType  FdTable::getType(int const fd) const
{
  return (this->_table[fd].first);
}

/*
**  WARNING! This method causes undefined behaviour if fd >= _table size
*/

/*
** Serializers
*/

uintptr_t
FdTable::_serializeListenSock(std::vector< ServerConfig const * > *  ptr)
{
	return reinterpret_cast<uintptr_t>(ptr);
}

uintptr_t
FdTable::_serializeConnSock(ConnectionData *  ptr)
{
	return reinterpret_cast<uintptr_t>(ptr);
}

uintptr_t
FdTable::_serializeFile(FileData *  ptr)
{
	return reinterpret_cast<uintptr_t>(ptr);
}

uintptr_t
FdTable::_serializePipe(CgiData *  ptr)
{
	return reinterpret_cast<uintptr_t>(ptr);
}

/*
** Deserializers
*/

std::vector< ServerConfig const * > *
FdTable::_deserializeListenSock(uintptr_t raw)
{
	return reinterpret_cast<std::vector< ServerConfig const * > * >(raw);
}

ConnectionData *
FdTable::_deserializeConnSock(uintptr_t raw)
{
	return reinterpret_cast<ConnectionData * >(raw);
}

FileData *
FdTable::_deserializeFile(uintptr_t raw)
{
	return reinterpret_cast<FileData * >(raw);
}

CgiData *
FdTable::_deserializePipe(uintptr_t raw)
{
	return reinterpret_cast<CgiData * >(raw);
}

/*
** Get specific type
*/

std::vector< ServerConfig const * > & FdTable::getListenSockData(int const fd)
{
  return (*this->_deserializeListenSock(this->_table[fd].second));
}

ConnectionData & FdTable::getConnSock(int const fd)
{
  return (*this->_deserializeConnSock(this->_table[fd].second));
}

FileData & FdTable::getFile(int const fd)
{
  return (*this->_deserializeFile(this->_table[fd].second));
}

CgiData & FdTable::getPipe(int const fd)
{
  return (*this->_deserializePipe(this->_table[fd].second));
}

/*
**  The right addFd method will be called depending on the argument data types.
**
**  fd type can be inferred by the data type.
*/

bool	FdTable::_littleAddChecker(int const fd)
{
  if (fd < 3) //Invalid fd. 0 - 2 are reserved for STD I/O.
    return (false);
  //Increase _table vector size if needed
  if (this->_table.size() <= static_cast<size_t>(fd))
    this->_table.resize(fd + 10);
  /*
  **  fd is already in use.
  **
  **  Each time an fd is closed, its type in _table must be set to None
  **  and its data must be deleted.
  */
  if (this->_table[fd].first != None)
    return (false);
  return (true);
}

bool
FdTable::add(int const fd, std::vector< ServerConfig const * > * data)
{
  if (!this->_littleAddChecker(fd))
	return (false);
  this->_table[fd].first = ListenSock;
  this->_table[fd].second = this->_serializeListenSock(data);
  return (true);
}

bool
FdTable::add(int const fd, ConnectionData * data)//Connection Data
{
	if (!this->_littleAddChecker(fd))
		return (false);
	this->_table[fd].first = ConnSock;
	this->_table[fd].second = this->_serializeConnSock(data);
	return (true);
}

bool
FdTable::add(int const fd, FileData * data)//File
{
	if (!this->_littleAddChecker(fd))
		return (false);
	this->_table[fd].first = File;
	this->_table[fd].second = this->_serializeFile(data);
	return (true);
}

bool
FdTable::add(int const fd, CgiData * data)//Pipes
{
	if (!this->_littleAddChecker(fd))
		return (false);
	this->_table[fd].first = Pipe;
	this->_table[fd].second = this->_serializePipe(data);
	return (true);
}

void  FdTable::remove(int const fd)
{
  /*
  **  Cannot store a pointer for second because we would need to know
  **  its type in advance.
  */
  FdType *  fdType;

  if (fd < 3 || this->_table.size() <= static_cast<size_t>(fd))
    return ;
  fdType = &this->_table[fd].first;
  if (*fdType == ListenSock)
    delete this->_deserializeListenSock(this->_table[fd].second);
  if (*fdType == ConnSock)
    delete this->_deserializeConnSock(this->_table[fd].second);
  if (*fdType == File)
    delete this->_deserializeFile(this->_table[fd].second);
  if (*fdType == Pipe)
    delete this->_deserializePipe(this->_table[fd].second);

  this->_table[fd].second = 0;
  *fdType = None;
  return ;
}
