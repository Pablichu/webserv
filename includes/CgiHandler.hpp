#pragma once

#include <map>
#include <algorithm>
#include <stdlib.h>

#include "Data.hpp"
#include "HttpInfo.hpp"
#include "UrlParser.hpp"
#include "utils.hpp"

class CgiHandler
{
private:

  void  _deleteEnv(std::vector<char *> & env);
  void  _addEnvVar(std::vector<char *> & env, std::string const & var);
  void  _execProgram(CgiData const & cgiData, std::vector<char *> env);
  bool  _parseCgiResponse(std::string & buff, int const buffSize,
                          std::map<std::string, std::string> & header);
  bool  _redirect(std::string & buff,
                  std::map<std::string, std::string> const & header,
                  std::size_t & rspSize, std::string & localPath);
  void  _addProtocolHeaders(std::string & buff,
                          std::map<std::string, std::string> const & header,
                          std::size_t & rspSize);
  void  _addBody(std::string & buff, std::string const & body);
  bool  _document(std::string & buff,
                  std::map<std::string, std::string> const & header,
                  std::size_t & rspSize);
  bool  _reWriteResponse(std::string & buff,
                          std::map<std::string, std::string> const & header,
                          std::size_t & rspSize, std::string & localPath);
  bool  _buildHttpHeaders(std::string & buff, int const buffSize,
                          std::size_t & rspSize, std::string & localPath);

public:

  CgiHandler(void);
  ~CgiHandler(void);

  std::vector<char *> *
  getEnv(std::map<std::string, std::string> const & reqHeader,
          std::map<std::string, std::string> const & urlData,
          std::string const & ip);
  bool	receiveData(int rPipe, ConnectionData & connData);
  bool  sendBody(int wPipe, ConnectionData & connData);
  bool  initPipes(CgiData & cgiData, std::vector<char *> & env);

};
