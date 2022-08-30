#include "UrlParser.hpp"

UrlParser::UrlParser(void) {}

UrlParser::~UrlParser(void) {}

/**/

bool  UrlParser::parse(std::string url,
                        std::map<std::string, std::string> & data)
{
  std::size_t needle;

  needle = url.find("?");
  if (needle != std::string::npos)
  {
    data["QUERY_STRING"] = url.substr(needle + 1);
    url.erase(needle, std::string::npos);
  }
  needle = url.find(".");
  if (needle != std::string::npos)
  {
    needle = url.find("/", needle);
    data["PATH"] = url.substr(0, needle);
    url.erase(0, needle);
    needle = data["PATH"].rfind("/");
    data["FILENAME"] = data["PATH"].substr(needle + 1);
    data["FILETYPE"] = data["FILENAME"].substr(data["FILENAME"].rfind(".") + 1);
  }
  else
  {
    data["PATH"] = url.substr();
    url.clear();
  }
  if (!url.empty())
  {
    data["PATH_INFO"] = url.substr();
    url.clear();
  }  
  return (true);
}
