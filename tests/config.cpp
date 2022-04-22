#include "Config.hpp"

void  printSet(std::set<std::string> const & s)
{
  std::set<std::string>::const_iterator it;

  for (it = s.begin(); it != s.end(); ++it)
  {
    if (*it != *(s.rbegin()))
      std::cout << *it << ", ";
    else
      std::cout << *it;
  }
  std::cout << '\n';
  return ;
}

void  printLocation(std::vector<LocationConfig> const & loc)
{
  std::vector<LocationConfig>::const_iterator it;
  std::size_t                                 i;

  i = 1;
  for (it = loc.begin(); it != loc.end(); ++it, ++i)
  {
    std::cout << "\nlocation # " << i << "\n\n";
    std::cout << "uri: " << it->uri << '\n';
    std::cout << "root: " << it->root << '\n';
    std::cout << "methods: ";
    printSet(it->methods);
    std::cout << "redirection: " << it->redirection << '\n';
    std::cout << std::boolalpha << "dir_list: " << it->dir_list << '\n';
    std::cout << "default_file: " << it->default_file << '\n';
    std::cout << "upload_dir: " << it->upload_dir << '\n';
    std::cout << "cgi_dir: " << it->cgi_dir << '\n';
  }
  return ;
}

void  printConf(std::vector<ServerConfig> const & cf)
{
  std::vector<ServerConfig>::const_iterator it;
  std::size_t                               i;

  i = 1;
  for (it = cf.begin(); it != cf.end(); ++it, ++i)
  {
    std::cout << "\nSERVER # " << i << "\n\n";
    std::cout << "port: " << it->port << '\n';
    std::cout << "server_name: ";
    printSet(it->server_name);
    std::cout << "not_found_page: " << it->not_found_page << '\n';
    std::cout << "max_body_size: " << it->max_body_size << '\n';
    std::cout << "location:\n";
    printLocation(it->location);
  }
  std::cout << std::endl;
  return ;
}

int main(int argc, char **argv)
{
  Config  conf;

  if (argc > 2)
  {
    std::cout << "Too many arguments. Max. one argument for config file path.";
    std::cout << std::endl;
    return (1);
  }
  if (argc == 2) // User provided config file path
    conf.setPath(argv[1]);
  if (!conf.parseFile())
  {
    std::cout << "Invalid config file." << std::endl;
    return (1);
  }
  printConf(conf.getConfig());
  return (0);
}
