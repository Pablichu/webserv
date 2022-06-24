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
    /*
    **  More types
    **
    **  else if (it->first == OtherType)
    **  else if (it->first == OtherOtherType)
    **  ...
    */
    it->first = None;
    it->second = 0;
  }
  return ;
}

uintptr_t
FdTable::_serializeListenSock(std::vector< ServerConfig const * > *  ptr)
{
	return reinterpret_cast<uintptr_t>(ptr);
}

std::vector< ServerConfig const * > *
FdTable::_deserializeListenSock(uintptr_t raw)
{
	return reinterpret_cast<std::vector< ServerConfig const * > * >(raw);
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

std::vector< ServerConfig const * > & FdTable::getListenSockData(int const fd)
{
  return (*this->_deserializeListenSock(this->_table[fd].second));
}

/*
**  The right addFd method will be called depending on the argument data types.
**
**  fd type can be inferred by the data type.
*/

bool
FdTable::add(int const fd, std::vector< ServerConfig const * > * data)
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
  this->_table[fd].first = ListenSock;
  this->_table[fd].second = this->_serializeListenSock(data);
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
  /*
  **  More types
  **
  **  else if (*fdType == OtherType)
  **  else if (*fdType == OtherOtherType)
  **  ...
  */
  this->_table[fd].second = 0;
  *fdType = None;
  return ;
}
