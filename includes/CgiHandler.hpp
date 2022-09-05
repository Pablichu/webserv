#pragma once

#include <map>
#include <algorithm>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>

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
  bool  _parseCgiResponse(InputOutput & io,
                          std::map<std::string, std::string> & header);
  bool  _redirect(ConnectionData & connData,
                  std::map<std::string, std::string> const & header,
                  std::string & localPath);
  void  _addProtocolHeaders(ConnectionData & connData,
                          std::map<std::string, std::string> const & header);
  void  _addBody(InputOutput & io, std::string const & body);
  bool  _document(ConnectionData & connData,
                  std::map<std::string, std::string> const & header);
  bool  _reWriteResponse(ConnectionData & connData,
                          std::map<std::string, std::string> const & header,
                          std::string & localPath);
  bool  _buildHttpHeaders(ConnectionData & connData, std::string & localPath);

public:

  CgiHandler(void);
  ~CgiHandler(void);

  int   getExitStatus(pid_t const pID);
  void  terminateProcess(pid_t const pID);
  std::vector<char *> *
        getEnv(std::map<std::string, std::string> const & reqHeader,
                std::map<std::string, std::string> const & urlData,
                std::string const & locationRoot,
                std::string const & ip);
  bool	receiveData(int rPipe, ConnectionData & connData);
  bool  sendBody(int wPipe, ConnectionData & connData);
  bool  initPipes(CgiData & cgiData, std::vector<char *> & env);

};
