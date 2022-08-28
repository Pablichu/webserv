#include "HttpInfo.hpp"

std::string const HttpInfo::protocol = "HTTP/1.1";

InitStatusCode::InitStatusCode(void)
{
  this->m.insert(
    std::pair<int const, std::string const>(200, "OK"));
  this->m.insert(
    std::pair<int const, std::string const>(201, "Created"));
  this->m.insert(
    std::pair<int const, std::string const>(301, "Moved Permanently"));
  this->m.insert(
    std::pair<int const, std::string const>(302, "Found"));
  this->m.insert(
    std::pair<int const, std::string const>(303, "See Other"));
  this->m.insert(
    std::pair<int const, std::string const>(308, "Permanent Redirect"));
  this->m.insert(
    std::pair<int const, std::string const>(400, "Bad Request"));
  this->m.insert(
    std::pair<int const, std::string const>(404, "Not Found"));
  this->m.insert(
    std::pair<int const, std::string const>(405, "Method Not Allowed"));
  this->m.insert(
    std::pair<int const, std::string const>(413, "Payload Too Large"));
  this->m.insert(
    std::pair<int const, std::string const>(414, "URI Too Long"));
  this->m.insert(
    std::pair<int const, std::string const>(415, "Unsupported Media Type"));
  this->m.insert(
    std::pair<int const, std::string const>(500, "Internal Server Error"));
  this->m.insert(
    std::pair<int const, std::string const>(503, "Service Unavailable"));
  this->m.insert(
    std::pair<int const, std::string const>(505, "HTTP Version Not Supported"));
}

std::map<int const, std::string const> const
  HttpInfo::statusCode(InitStatusCode().m);

InitContentType::InitContentType(void)
{
  this->m.insert(
    std::pair<std::string const, std::string const>(".css", "text/css"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".gif", "image/gif"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".html", "text/html"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".htm", "text/html"));
  this->m.insert(
    std::pair<std::string const, std::string const>(
      ".ico", "image/vnd.microsoft.icon"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".jpeg", "image/jpeg"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".jpg", "image/jpeg"));
  this->m.insert(
    std::pair<std::string const, std::string const>(
      ".js", "application/javascript"));
  this->m.insert(
    std::pair<std::string const, std::string const>(
      ".json", "application/json"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".mp3", "audio/mpeg"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".png", "image/png"));
  this->m.insert(
    std::pair<std::string const, std::string const>(
      ".rar", "application/vnd.rar"));
  this->m.insert(
    std::pair<std::string const, std::string const>(
      ".tar", "application/x-tar"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".txt", "text/plain"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".wav", "audio/wav"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".xml", "application/xml"));
  this->m.insert(
    std::pair<std::string const, std::string const>(".zip", "application/zip"));
  this->m.insert(
    std::pair<std::string const, std::string const>(
      ".7z", "application/x-7z-compressed"));
}

std::map<std::string const, std::string const> const
  HttpInfo::contentType(InitContentType().m);