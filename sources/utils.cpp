#include "utils.hpp"

std::string utils::getFileExtension(std::string const & input)
{
  return (input.substr(input.rfind(".")));
}
