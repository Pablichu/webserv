#pragma once

#include <iostream>
#include <sstream>

#include "Data.hpp"

namespace utils
{
  template< typename T >
  std::string toString(T input)
  {
    std::ostringstream  ss;

    ss << input;
    return (ss.str());
  }

  std::string getFileExtension(std::string const & input);
  std::string extractHost(std::string const & input);
  std::string extractPort(std::string const & input);
  std::string getDate(void);
  void        addContentLengthHeader(std::string & rspContent,
                                      std::size_t const endHeadersPos);
  void        addKeepAliveHeaders(std::string & headers,
                                  int const HandledRequests,
                                  bool const clientClose);
}
