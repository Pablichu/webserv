#include "webserv.hpp"

int		main(int argc, char **argv)
{
  Config  config;
	Server  server;

  if (argc != 2)
  {
    std::cout << "Need config file path as argument. Nothing else.";
    std::cout << std::endl;
    return (1);
  }
  config.setPath(argv[1]);
  if (!config.parseFile())
  {
    std::cout << "Invalid config file." << std::endl;
    return (1);
  }
  signal(SIGCHLD, SIG_IGN); // provisonal, should check exit code of each child process
  if (!server.prepare(config.getConfig()))
    return (1);
  if (!server.start())
    return (1);
  return (0);
}
