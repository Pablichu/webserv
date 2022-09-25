#pragma once

#include <cstdlib>

#include "Data.hpp"
#include "utils.hpp"

class ConfigMatcher
{

public:

  ConfigMatcher(void);
  ~ConfigMatcher(void);

  bool  match(ConnectionData & connData);

private:

  void  _matchServer(std::vector<ServerConfig const *> & servers,
                      std::size_t & index, std::string const & reqHost);
  bool	_matchLocation(std::vector<LocationConfig> const & servers,
												std::size_t & index, std::string const & reqUri);

};
