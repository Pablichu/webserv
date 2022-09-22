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

std::string utils::decToHex(std::size_t dec)
{
  std::stringstream ss;

  ss<< std::hex << dec;
  return (ss.str());
}

/*
**  Inserts Content-Length header to rspContent, which includes
**  headers and body, or just headers if no body will be sent.
**
**  endHeadersPos is one position after the last \n of \r\n\r\n
**  which signals the end of headers, and start of body, if present.
*/

void  utils::addContentLengthHeader(std::string & rspContent,
                                    std::size_t const endHeadersPos)
{
  rspContent.insert(endHeadersPos - 2, "Content-Length: "
                  + utils::toString(rspContent.length() - endHeadersPos)
                  + "\r\n");
  return ;
}

/*
**  HandledRequests does not count the current one.
**
**  clientClose means client sent "Connection: close" header.
*/

void  utils::addKeepAliveHeaders(std::string & headers,
                                int const handledRequests,
                                bool const clientClose)
{
  if (handledRequests == ConnectionData::keepAliveMaxReq - 1
      || clientClose)
  {
    headers.append("Connection: close\r\n");
  }
  else
  {
    headers.append("Connection: keep-alive\r\n");
    headers.append("Keep-Alive: timeout=");
    headers.append(utils::toString<int>(
                    static_cast<int>(ConnectionData::keepAliveTimeout)
                  ) + ", ");
    headers.append("max=" +
                    utils::toString<int>(ConnectionData::keepAliveMaxReq));
    headers.append("\r\n");
  }
  return ;
}
