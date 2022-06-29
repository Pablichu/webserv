#include "webserv.hpp"
#include <assert.h>

/*
**  Using real fds, otherwise the destructor's close function gives problems.
**  Monitor's destructor closes open fds.
*/

void  increase(void)
{
  Monitor     mon;
  std::size_t i;
  int         fd;

  for (i = 0; i < 1000; ++i)
  {
    fd = open("./tests/monitor.cpp", O_RDONLY);
    mon.add(fd, POLLIN);
    assert(mon.len() == i + 1);
  }
  assert(mon.len() == i);
  assert(mon[0].fd > 2 && (mon[0].events & POLLIN));
  assert(mon[500].fd > mon[0].fd && (mon[500].events == mon[0].events));
  return ;
}

/**
* Here it is not necessary to use real fds as remove() and purge()
* do not call close() internally.
*/

void  remove(void)
{
  Monitor     mon;
  std::size_t i;

  for (i = 0; i < 10; ++i)
  {
    mon.add(i, POLLIN);
  }
  mon.remove(i); // BAD index
  for (i = 0; i < mon.len(); ++i)
  {
    mon.remove(i);
  }
  mon.purge();
  assert(mon.len() == 0);
  return ;
}

int main()
{
  std::cout << "\n--- MONITOR TESTS ---\n";
  increase();
  std::cout << "\nINCREASE: OK\n";
  remove();
  std::cout << "\nREMOVE: OK\n";
  std::cout << std::endl;
  return (0);
}
