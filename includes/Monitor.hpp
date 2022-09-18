#pragma once

/*
**  Not including webserv.hpp because Server.hpp does not compile,
**  as it has Monitor declared inside and does not know at compile time
**  its size.
**  Another solution would be including Monitor * as a pointer, not having
**  to know Monitor's size at compile time.
*/
#include <unistd.h>
#include <poll.h>
#include <list>
#include <vector>
#include <algorithm>
#include <ostream>

#include <FdTable.hpp>

/*
**  Created so that a pollfd can be initialized with different
**  default values.
*/

struct  EmptyPollFd : public pollfd
{
  EmptyPollFd();
};

class Monitor
{

private:

  std::vector<pollfd> _fds;
  FdTable &           _fdTable;
  /*
  **  All fds associated to a client connection are stored here,
  **  and used by _readjustTableRefs on each reallocation of a Monitor instance.
  */
  std::list<int>      _connFds;
  // Improves Monitor iteration and size efficiency
  int                 _biggestActiveFd;

  Monitor(void);
  void        _updateTableRefs(void);
  void        _getNextBiggestFd(void);

public:

  // This pollfd is passed to _fds resize method as default value.
  static pollfd const  emptyPollFd;

  Monitor(FdTable & fdTable);
  ~Monitor(void);

  pollfd &    operator[](std::size_t index);

  pollfd *    getFds(void);
  std::size_t len(void) const;
  int         biggestActiveFd(void) const;
  void        add(int const fd, short const events, bool connFd);
  void        remove(int const fd);

};

std::ostream &  operator<<(std::ostream & out, Monitor & monitor);
