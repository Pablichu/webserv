#include "ConfigMatcher.hpp"

ConfigMatcher::ConfigMatcher(void)
{}

ConfigMatcher::~ConfigMatcher(void)
{}

bool  ConfigMatcher::match(ConnectionData & connData)
{
  std::string path = connData.urlData.find("PATH")->second;

  this->_matchServer(
    *(connData.portConfigs),
    connData.serverIndex, utils::extractHost(connData.req.getPetit("HOST")));
  if (connData.urlData.count("FILENAME"))
    path.erase(path.rfind("/"));
  if (!this->_matchLocation(
      connData.getServer()->location,
      connData.locationIndex, path))
    return (false);
  return (true);
}

void  ConfigMatcher::_matchServer(std::vector<ServerConfig const *> & servers,
                              std::size_t & index, std::string const & reqHost)
{
  int                                         match;
  std::vector<ServerConfig const *>::iterator it;
  std::size_t                                 i;

  match = -1;
  for (it = servers.begin(), i = 0; it != servers.end(); ++it, ++i)
  {
    if ((*it)->server_name.empty() && match == -1)
    {
      index = i;
      match = 0;
    }
    else if ((*it)->server_name.count(reqHost))
    {
      index = i;
      match = 1;
      break ;
    }
  }
  if (match == -1)
    index = 0;
  return ;
}

bool  ConfigMatcher::_matchLocation(std::vector<LocationConfig> const & locations,
                              std::size_t & index, std::string const & reqUri)
{
  long                                        uriLen;
  std::vector<LocationConfig>::const_iterator it;
  std::size_t                                 i;

  uriLen = 0;
  for (it = locations.begin(), i = 0; it != locations.end(); ++it, ++i)
  {
    if (reqUri.find(it->uri) == 0)
    {
      if (static_cast<long>(reqUri.length()) > uriLen)
      {
        uriLen = it->uri.length();
        index = i;
      }
    }
  }
  if (static_cast<long>(reqUri.length()) != uriLen)
  {
    //Path not found
    if (! (std::labs(static_cast<long>(reqUri.length()) - uriLen) == 1
            && reqUri[reqUri.length() - 1] == '/') )
      return (false);
  }
  return (true);
}
