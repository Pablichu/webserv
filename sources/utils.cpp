#include "utils.hpp"

/*
**  IMPORTANT!!
**  It must be called only when it is known that
**  a file name is present in the input.
*/

std::string utils::getFileExtension(std::string const & input)
{
  return (input.substr(input.rfind(".")));
}

/*
**  IMPORTANT!!
**  input must be the header HOST, which contains host:port value.
*/

std::string utils::extractHost(std::string const & input)
{
  return (input.substr(0, input.rfind(":")));
}

/*
**  IMPORTANT!!
**  input must be the header HOST, which contains host:port value.
*/

std::string utils::extractPort(std::string const & input)
{
  return (input.substr(input.rfind(":") + 1));
}
