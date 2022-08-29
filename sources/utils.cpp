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

void  utils::removeWhiteSpace(std::string & input)
{
  std::size_t needle;

  needle = input.find_last_not_of(' ');
  //Remove trailing white space
  if (needle != input.length() - 1)
    input.erase(needle + 1);
  //Reduce inner white space
  while (true)
  {
    needle = input.find("  ");
    if (needle == std::string::npos)
      break ;
    input.erase(needle + 1, input.find_first_not_of(' ', needle));
  }
  return ;
}
