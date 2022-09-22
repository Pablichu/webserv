#include "HttpValidator.hpp"

HttpValidator::HttpValidator(void) {}

HttpValidator::~HttpValidator(void) {}

bool  HttpValidator::_matchConfig(ConnectionData & connData, int & error)
{
  if (!this->_configMatcher.match(connData))
  { //Request path did not match any server's location uri
    error = 404; // Not Found
    return (false);
  }
  return (true);
}

bool  HttpValidator::_validMethod(ConnectionData & connData, int & error)
{
  LocationConfig const *  location = connData.getLocation();
  std::string const &     method = connData.req.getPetit("METHOD");

  if (method != "GET" && method != "POST" && method != "DELETE")
  {
    error = 501; // Not Implemented
    return (false);
  }
  if (!location->methods.count(method))
  { //Request method is not allowed in target location
    error = 405; // Method Not Allowed
    return (false);
  }
  return (true);
}

bool  HttpValidator::_validHttpVersion(std::string const & version, int & error)
{
  if (version != HttpInfo::protocol)
  {
    error = 505; // HTTP Version Not Supported
    return (false);
  }
  return (true);
}

std::string HttpValidator::_extractMediaType(std::string const & input)
{
  std::string mediaType;

  // If no ';' is found, all input is assigned to mediaType
  mediaType = input.substr(0, input.find(';'));
  return (mediaType);
}

bool  HttpValidator::_validContentType(ConnectionData & connData, int & error)
{
  std::string                                 fileExtension;
  std::string                                 contentType;
  std::string                                 mediaType;
  bool                                        isCgi;
  std::map<std::string, std::string> const &  urlData = connData.urlData;

  if (!urlData.count("FILETYPE"))
    return (true);
  fileExtension = urlData.at("FILETYPE");
  // count returns 0 or 1, which correspond to false or true respectively
  isCgi = connData.getServer()->cgiInterpreter.count(fileExtension);
  if (!HttpInfo::contentType.count(fileExtension)
      && !isCgi)
    error = 415; // Unsupported Media Type
  else if (connData.req.getPetit("METHOD") == "POST")
  {
    contentType = connData.req.getPetit("CONTENT-TYPE");
    if (contentType == "")
    {
      error = 400; // Bad Request
      return (false);
    }
    mediaType = this->_extractMediaType(contentType);
    if (isCgi)
    {
      if (mediaType != "text/plain" && mediaType != "multipart/form-data"
          && mediaType != "application/x-www-form-urlencoded")
        error = 400;
    }
    else
    {
      if (mediaType != HttpInfo::contentType.at(fileExtension))
        error = 400;
    }
  }
  if (error >= 400)
    return (false);
  return (true);
}

bool  HttpValidator::_validContentLength(ConnectionData & connData, int & error)
{
  Request & req = connData.req;

  if (req.getPetit("METHOD") == "POST")
  {
    if (req.getPetit("CONTENT-LENGTH") == "")
    {
      if (req.getPetit("TRANSFER-ENCODING") != "chunked")
      {
        error = 411; // Length Required
        return (false);
      }
    }
  }
  return (true);
}

bool  HttpValidator::_validUriLength(std::string const & uri, int & error)
{
  // Maximum uri length in most browsers
  if (uri.length() > 2000)
  {
    error = 414; // Uri Too Long
    return (false);
  }
  return (true);
}

bool  HttpValidator::isValidRequest(ConnectionData & connData, int & error)
{
  if (!this->_matchConfig(connData, error))
    return (false);
  if (!this->_validMethod(connData, error))
    return (false);
  if (!this->_validHttpVersion(connData.req.getPetit("PROTOCOL"), error))
    return (false);
  if (!this->_validContentType(connData, error))
    return (false);
  if (!this->_validContentLength(connData, error))
    return (false);
  if (!this->_validUriLength(connData.req.getPetit("PATH"), error))
    return (false);
  return (true);
}
