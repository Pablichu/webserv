#include "Monitor.hpp"

EmptyPollFd::EmptyPollFd()
{
  this->fd = -1;
  return ;
}

const pollfd  Monitor::emptyPollFd = EmptyPollFd();

Monitor::Monitor(FdTable & fdTable) : _fdTable(fdTable),
                                      _biggestActiveFd(0)
{
  return ;
}

Monitor::~Monitor(void)
{
  std::vector<pollfd>::iterator it;

  for (it = this->_fds.begin(); it != this->_fds.end(); ++it)
  {
    if (it->fd != -1)
      close(it->fd);
  }
  this->_fds.clear();
}

struct pollfd & Monitor::operator[](std::size_t index)
{
  return (this->_fds[index]);
}

struct pollfd * Monitor::getFds(void)
{
  return (&this->_fds.front());
}

std::size_t Monitor::len(void) const
{
  return (this->_fds.size());
}

int Monitor::biggestActiveFd(void) const
{
  return (this->_biggestActiveFd);
}

/*
**  Add an fd alongside the events that poll will track to _fds.
**
**  If Monitor needs to be resized, a reallocation will happen.
**
**  _biggestActiveFd is stored and updated to improve Monitor iteration and
**  size efficiency.
*/

void  Monitor::add(int const fd, short const events)
{
  if (static_cast<std::size_t>(fd) >= this->_fds.size())
    this->_fds.resize(fd * 2, Monitor::emptyPollFd);
  this->_fds[fd].fd = fd;
  this->_fds[fd].events = events;
  if (fd > this->_biggestActiveFd)
    this->_biggestActiveFd = fd;
  return ;
}

void  Monitor::_getNextBiggestFd(void)
{
  for (std::size_t i = this->_biggestActiveFd - 1;
        i > 2; --i) // 2 is fd for stderr
  {
    if (this->_fds[i].fd > 2)
    {
      this->_biggestActiveFd = this->_fds[i].fd;
      break ;
    }
  }
}

/*
**  Mark a pollfd as empty by setting its fd to -1.
**
**  If it was the biggest fd in _fds, _biggestActiveFd is updated.
**
**  If the size of _fds is greater than two times the value of
**  the new _biggestActiveFd, a shrinking reallocation of _fds is applied.
*/

void  Monitor::remove(int const fd)
{
  if (static_cast<std::size_t>(fd) >= this->_fds.size())
    return ;
  this->_fds[fd].fd = -1;
  if (fd == this->_biggestActiveFd)
    this->_getNextBiggestFd();
  if (this->_fds.size()
      > static_cast<std::size_t>(this->_biggestActiveFd * 2))
    this->_fds.resize(this->_biggestActiveFd * 2);
  return ;
}

std::ostream &  operator<<(std::ostream & out, Monitor & monitor)
{
  std::size_t i;

  for (i = 0; i < monitor.len(); ++i)
  {
    out << monitor[i].fd << std::endl;
  }
  return (out);
}
