#include "UrlParser.hpp"

UrlParser::UrlParser(void) {}

UrlParser::~UrlParser(void) {}

/**/

bool  UrlParser::parse(std::string & url,
                        std::map<std::string, std::string> & data)
{
  std::size_t needle;

  needle = url.find("?");
  if (needle != std::string::npos)
  {
    data["Query_String"] = url.substr(needle + 1);
    url.erase(needle, std::string::npos);
  }
  needle = url.find(".");
  if (needle != std::string::npos)
  {
    needle = url.find("/", needle);
    data["Path"] = url.substr(0, needle);
    url.erase(0, needle);
    needle = data["Path"].rfind("/");
    data["FileName"] = data["Path"].substr(needle + 1);
  }
  else
  {
    data["Path"] = url.substr();
    url.clear();
  }
  if (!url.empty())
  {
    data["Path_Info"] = url.substr();
    url.clear();
  }  
  return (true);
}
