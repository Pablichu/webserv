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
