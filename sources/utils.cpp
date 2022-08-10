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

/*
**  Returns current date in RFC 822 format
*/

std::string utils::getDate(void)
{
  time_t      rawTime;
  tm  *       timePtr;
  int const   dateLen = 30;
  char        date[dateLen + 1];
  std::string res;

  std::fill(date, date + dateLen + 1, 0);
  time(&rawTime);
  timePtr = gmtime(&rawTime);
  strftime(date, dateLen + 1, "%a, %d %b %Y %X ", timePtr);
  res = date;
  res.append("GMT");
  return (res);
}
