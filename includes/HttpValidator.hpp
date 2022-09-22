#pragma once

#include "HttpInfo.hpp"
#include "ConfigMatcher.hpp"

class HttpValidator
{

private:

  ConfigMatcher _configMatcher;

  bool  _matchConfig(ConnectionData & connData, int & error);
  bool  _validMethod(ConnectionData & connData, int & error);
  bool  _validHttpVersion(std::string const & version, int & error);
  std::string _extractMediaType(std::string const & input);
  bool  _validContentType(ConnectionData & connData, int & error);
  bool  _validContentLength(ConnectionData & connData, int & error);
  bool  _validUriLength(std::string const & uri, int & error);

public:

  HttpValidator(void);
  ~HttpValidator(void);

  bool  isValidRequest(ConnectionData & connData, int & error);

};
