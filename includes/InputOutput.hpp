#pragma once

#include <string>
#include <iostream>

class InputOutput
{
private:

	std::size_t	_totalBytesRead;
	std::size_t	_totalBytesSent;
	std::size_t	_payloadSize;
	std::size_t	_buffSize;
	std::string	_buff;

public:

  static std::size_t const  buffCapacity = 8192;

  InputOutput(void);
  
  char *        inputBuffer(void);
  char *        outputBuffer(void);
  std::string & outputStrBuffer(void);
  void          pushBack(std::string & input);
  void          pushBack(std::string const & input);
  std::size_t   getBufferSize(void) const;
  std::size_t   getAvailableBufferSize(void) const;
  void          addBytesRead(std::size_t const bytes);
  void          addBytesSent(std::size_t const bytes);
  std::size_t   getPayloadSize(void) const;
  void          setPayloadSize(std::size_t const size);
  bool          finishedSend(void) const;
  bool          finishedRead(void) const;
  void          setFinishedSend(void);
  void          setFinishedRead(void);
  bool          isFirstRead(void) const;
  void          clear(void);
};
