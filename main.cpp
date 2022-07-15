#include "./includes/Server.hpp"

int		main(int argc, char **argv)
{
  Config  config;
	Server  server;

  if (argc == 1)
  {
    std::cout << " -> Default configuration" << std::endl;
    config.setPath(".default/default.json");
  }
  else if (argc == 2)
    config.setPath(argv[1]);
  else
  {
    std::cout << "Need config file path as argument. Nothing else." << std::endl;
	return (1);
  }
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
