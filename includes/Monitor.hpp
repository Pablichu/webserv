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

class Monitor
{

private:

  struct pollfd *         _fds;
  std::size_t             _len;
  std::size_t             _cap;
  std::list<std::size_t>  _removedIndexs;

public:

  Monitor(void);
  ~Monitor(void);

  struct pollfd & operator[](std::size_t index);

  struct pollfd * getFds(void);
  std::size_t     len(void) const;
  void	          add(int const fd, short const events);
  void	          remove(std::size_t index);
  void            purge(void);

  void	_increaseCap(void);
  bool  _getLastValidElem(struct pollfd & elem, std::size_t const currPos);

};
