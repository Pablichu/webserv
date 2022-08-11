#include "Data.hpp"

CgiData::CgiData(int const socket, std::string const & filePath)
                : socket(socket), filePath(filePath)
{
  std::fill(this->inPipe, this->inPipe + 2, 0);
  std::fill(this->outPipe, this->outPipe + 2, 0);
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

FileData::FileData(std::string const & filePath, int const socket)
                    : fd(0), socket(socket), filePath(filePath)
{
  return ;
}

ConnectionData::ConnectionData(void) : serverIndex(0), locationIndex(0),
  totalBytesRead(0), totalBytesSent(0), rspSize(0), rspBuffSize(0),
  rspBuffOffset(0), fileData(0), cgiData(0)
{
  /*
  **  To simulate a Buffer of 8K
  */
  this->rspBuff = std::string(ConnectionData::rspBuffCapacity + 1, 0);
  return ;
}

ConnectionData::~ConnectionData(void)
{
  return ;
}

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

