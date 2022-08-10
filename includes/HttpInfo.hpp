#pragma once

#include <map>
#include <iostream>

struct	InitStatusCode
{
	std::map<int const, std::string const>	m;

	InitStatusCode(void);
};

struct	InitContentType
{
	std::map<std::string const, std::string const>	m;

	InitContentType(void);
};

struct HttpInfo
{
  static std::string const																		protocol; // HTTP/1.1
	static std::map<int const, std::string const> const					statusCode;
	static std::map<std::string const, std::string const> const	contentType;
};
