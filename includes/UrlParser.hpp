#pragma once

#include <iostream>
#include <map>

class UrlParser
{
private:

public:

  UrlParser(void);
  ~UrlParser(void);

  bool  parse(std::string & url, std::map<std::string, std::string> & data);

};
