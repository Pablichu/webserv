#include "../includes/Config.hpp"

Config::Config(void) : _path("default/path")
{
  return ;
}

Config::~Config(void)
{
  return ;
}

std::string const & Config::getPath(void) const
{
  return (this->_path);
}

void  Config::setPath(std::string const & path)
{
  this->_path = path;
  return ;
}

bool  Config::isValid(void) const
{
  if (this->_path == ""
      || !this->_validPath()
      || !this->_validFile())
    return (false);
  return (true);
}

//PRIVATE METHODS

bool  Config::_validPath(void) const
{
  std::size_t   file_type_pos;
  std::ifstream file;

  //Check file type
  file_type_pos = this->_path.find(".json");
  if ( this->_path.length() <= 5
      || file_type_pos == std::string::npos
      || file_type_pos + 5 != this->_path.length() )
      return (false);
  //Check file exists
  file.open(this->_path.c_str());
  if (!file.is_open())
    return (false);
  file.close();
  return (true);
}

bool  Config::_validFile(void) const
{
  //PENDING...
  return (true);
}