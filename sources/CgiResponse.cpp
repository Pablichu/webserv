#include "CgiResponse.hpp"

CgiResponse::CgiResponse(int socket, std::size_t connIndex) : _socket(socket),
                          _connIndex(connIndex)
{
  std::fill(this->_inPipe, this->_inPipe + 2, 0);
  std::fill(this->_outPipe, this->_outPipe + 2, 0);
  return ;
}

CgiResponse::~CgiResponse(void)
{
  return ;
}

int CgiResponse::getSocket(void) const
{
  return (this->_socket);
}

std::size_t CgiResponse::getIndex(void) const
{
  return (this->_connIndex);
}

int CgiResponse::getRInPipe(void) const
{
  return (this->_inPipe[0]);
}

int CgiResponse::getWInPipe(void) const
{
  return (this->_inPipe[1]);
}

int CgiResponse::getROutPipe(void) const
{
  return (this->_outPipe[0]);
}

int CgiResponse::getWOutPipe(void) const
{
  return (this->_outPipe[1]);
}

// Provisional

void  CgiResponse::_execProgram(ConnectionData & connData)
{
  std::string const path = connData.req.getPetit("Path");
  std::string       programPath;
  char **           argv;

  programPath = connData.getLocation().cgi_dir + '/';
  programPath.append(path.substr(path.find_last_of("/") + 1,
                      path.length() - path.find_last_of("/") + 1));
  argv = new char *[1 + 1];
  argv[1] = 0;
  argv[0] = const_cast<char *>(programPath.c_str());
  dup2(this->_inPipe[0], STDIN_FILENO);
  close(this->_inPipe[0]);
  dup2(this->_outPipe[1], STDOUT_FILENO);
  close(this->_outPipe[1]);
  execve(programPath.c_str(), argv, NULL); //Third argument must be the cgi env variables
  delete [] argv;
}

bool  CgiResponse::initPipes(ConnectionData & connData)
{
  pid_t child;

  if (pipe(this->_inPipe) == -1
      || pipe(this->_outPipe) == -1)
    return (false);
  child = fork();
  if (child < 0)
    return (false);
  if (!child)
  { //Child process
    close(this->_inPipe[1]);
    close(this->_outPipe[0]);
    this->_execProgram(connData);
    std::cout << "exec failed" << std::endl;
    exit(EXIT_FAILURE);
  }
  else
  { //Parent process
    close(this->_inPipe[0]);
    close(this->_outPipe[1]);
    return (true);
  }
}
