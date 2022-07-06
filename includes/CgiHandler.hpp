#pragma once

#include "webserv.hpp"

struct  CgiData;
struct  ConnectionData;

class CgiHandler
{
private:

  void  _deleteEnv(std::vector<char *> & env);
  void  _addEnvVar(std::vector<char *> & env, std::string const & var);
  void  _execProgram(CgiData const & cgiData, ConnectionData & connData,
                      std::vector<char *> env);

public:

  CgiHandler(void);
  ~CgiHandler(void);

  std::vector<char *> *
  getEnv(std::map<std::string, std::string> const & reqHeader,
          std::map<std::string, std::string> const & urlData,
          std::string const & ip);
  bool	receiveData(int rPipe, ConnectionData & connData);
  bool  initPipes(CgiData & cgiData, ConnectionData & connData,
                  std::vector<char *> & env);

};
