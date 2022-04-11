#include "webserv.hpp"

int main(void)
{
  Server  server;

  if (!server.prepare())
    return (1);
  if (!server.start())
    return (1);
  return (0);
}
