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

void  CgiResponse::_execProgram(void)
{
  char  progPath[] = "/tmp/www/localhost/cgi-bin/reply.cgi";
  char  *argv[2] = {progPath, 0};

  dup2(this->_inPipe[0], STDIN_FILENO);
  close(this->_inPipe[0]);
  dup2(this->_outPipe[1], STDOUT_FILENO);
  close(this->_outPipe[1]);
  execve(progPath, argv, NULL); //Thid argument must be the cgi env variables
}

bool  CgiResponse::initPipes(void)
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
    this->_execProgram();
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
