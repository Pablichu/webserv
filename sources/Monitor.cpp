#include "Monitor.hpp"

Monitor::Monitor(void) : _len(0), _cap(100)
{
  this->_fds = new struct pollfd [this->_cap];
  return ;
}

Monitor::~Monitor(void)
{
  this->purge();
  for (std::size_t i = 0; i < this->_len; ++i)
  {
    close(this->_fds[i].fd);
  }
  delete[] this->_fds;
}

struct pollfd & Monitor::operator[](std::size_t index)
{
  return (this->_fds[index]);
}

struct pollfd * Monitor::getFds(void)
{
  return (this->_fds);
}

std::size_t Monitor::len(void) const
{
  return (this->_len);
}

/*
**  Add an fd alonside the events that poll will track to the monitor array.
*/

void  Monitor::add(int const fd, short const events)
{
  struct pollfd * newElement;

  if (this->_len == this->_cap)
    this->_increaseCap();
  newElement = this->_fds + this->_len;
  newElement->fd = fd;
  newElement->events = events;
  this->_len += 1;
  return ;
}

/*
**  Mark an array element as removable when calling purge. 
*/

void  Monitor::remove(std::size_t index)
{
  if (index >= this->_len)
    return ;
  this->_fds[index].fd = -1;
  this->_removedIndexs.push_back(index);
  return ;
}

/*
**  Substitute removed element's content with that of valid elements at the back.
**  This can be done because the order of the array's elements
**  is not relevant for poll.
*/

void  Monitor::purge(void)
{
  std::list<std::size_t>::iterator  it;
  struct pollfd                     lastValidElem;

  if (this->_removedIndexs.empty())
    return ;
  for (it = this->_removedIndexs.begin();
        it != this->_removedIndexs.end(); ++it)
  {
    if (!this->_getLastValidElem(lastValidElem, *it))
      break ;
    this->_fds[*it].fd = lastValidElem.fd;
    this->_fds[*it].events = lastValidElem.events;
  }
  this->_removedIndexs.clear();
  return ;
}

/*
**  Increase capacity of monitor array by reassigning memory
**  to a new array and copying all elements of the previous array to
**  the new one.
*/

void  Monitor::_increaseCap(void)
{
  struct pollfd * aux;

  aux = new struct pollfd [this->_cap * 2];
  std::copy(this->_fds, this->_fds + this->_len, aux);
  delete[] this->_fds;
  this->_fds = aux;
  this->_cap *= 2;
  return ;
}

/*
**  Obtains the last valid element of the array that will substitute
**  a removed element.
*/

bool  Monitor::_getLastValidElem(struct pollfd & elem,
  std::size_t const currPos)
{
  struct pollfd * target;

  for (std::size_t i = this->_len - 1; i > currPos; --i)
  {
    target = &this->_fds[i];
    if (target->fd != -1)
    {
      elem.fd = target->fd;
      elem.events = target->events;
      target->fd = -1;
      this->_len = i;
      return (true);
    }
  }
  this->_len = currPos;
  return (false);
}
