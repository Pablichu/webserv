#include "Data.hpp"

CgiData::CgiData(pollfd & socket, std::string const & interpreterPath,
                  std::string const & scriptPath)
                : socket(socket), interpreterPath(interpreterPath),
                scriptPath(scriptPath)
{
  std::fill(this->inPipe, this->inPipe + 2, -1);
  std::fill(this->outPipe, this->outPipe + 2, -1);
  return ;
}

int CgiData::getRInPipe(void) const
{
  return (this->inPipe[0]);
}

int CgiData::getWInPipe(void) const
{
  return (this->inPipe[1]);
}

int CgiData::getROutPipe(void) const
{
  return (this->outPipe[0]);
}

int CgiData::getWOutPipe(void) const
{
  return (this->outPipe[1]);
}

void  CgiData::closeWInPipe(void)
{
  if (this->inPipe[1] == -1)
    return ;
  close(this->inPipe[1]);
  this->inPipe[1] = -1;
  return ;
}

void  CgiData::closeROutPipe(void)
{
  if (this->outPipe[0] == -1)
    return ;
  close(this->outPipe[0]);
  this->outPipe[0] = -1;
  return ;
}

void  CgiData::closePipes(void)
{
  this->closeWInPipe();
  this->closeROutPipe();
  return ;
}

FileData::FileData(std::string const & filePath, pollfd & socket)
                    : fd(0), socket(socket), filePath(filePath), rspStatus(200)
{
  return ;
}

ConnectionData::ConnectionData(void) : serverIndex(0), locationIndex(0),
  req(this->getServer()->max_body_size), handledRequests(0), dirListNeedle(0),
  fileData(0), cgiData(0)
{
  return ;
}

ConnectionData::~ConnectionData(void)
{
  return ;
}

double const  ConnectionData::timeout = 5;
int const ConnectionData::max = 100;

ServerConfig const *  ConnectionData::getServer(void)
{
  if (!this->portConfigs->size())
    return (0);
  return ((*this->portConfigs)[this->serverIndex]);
}

LocationConfig const *  ConnectionData::getLocation(void)
{
  if (!this->portConfigs->size())
    return (0);
  return (&this->getServer()->location[this->locationIndex]);
}

void  ConnectionData::setIdle(void)
{
  this->serverIndex = 0; // Maybe leave it set to enforce same server.
  this->locationIndex = 0;
  this->req.clear();
  this->ip.clear();
  this->urlData.clear();
  this->io.clear();
  this->status = Idle;
  this->lastActive = time(NULL);
  this->handledRequests += 1;
  this->dirListNeedle = 0; // Should have been zeroed previously
  this->fileData = 0; // Should have been zeroed previously
  this->cgiData = 0; // Should have been zeroed previously
  return ;
}

