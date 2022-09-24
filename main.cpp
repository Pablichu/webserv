
#include "./includes/Server.hpp"

void	leak_checker()
{
	system("leaks webserv");
}

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
  if (!server.prepare(config.getConfig()))
    return (1);
  server.start();
  //std::atexit(leak_checker);
  return (0);
}
