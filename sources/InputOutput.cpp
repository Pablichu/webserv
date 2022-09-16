#include "InputOutput.hpp"

InputOutput::InputOutput(void) : _totalBytesRead(0), _totalBytesSent(0),
  _payloadSize(0), _buffSize(0)
{
  /*
  **  To simulate a Buffer of 8K
  */
  this->_buff = std::string(InputOutput::buffCapacity + 1, 0);
  return ;
}

char *  InputOutput::inputBuffer(void)
{
  return (&this->_buff[this->_buffSize]);
}

char *  InputOutput::outputBuffer(void)
{
  return (&this->_buff[0]);
}

std::string & InputOutput::outputStrBuffer(void)
{
  return (this->_buff);
}

/*
**  pushAmount functionality is needed because there are parts of
**  the code, like in FileHandler's writefile method, where there is a need
**  to add as much input content as possible in each call, and it doesn't
**  matter if the size of input is bigger than the buffer size, as the contents
**  of input that are not moved to the buffer stay available in subsequent
**  calls.
**
**  In other situations where all of input's content must be moved to the
**  buffer, the caller must check the available buffer size to ensure
**  the entire input content will fit in the buffer.
*/

void  InputOutput::pushBack(std::string & input)
{
  std::size_t spaceAvailable;
  std::size_t pushAmount;

  if (this->_buffSize == InputOutput::buffCapacity)
    return ;
  spaceAvailable = this->getAvailableBufferSize();
  pushAmount = input.size() <= spaceAvailable ? input.size() : spaceAvailable;
  this->_buff.replace(this->_buffSize, pushAmount, input, 0, pushAmount);
  input.erase(0, pushAmount);
  this->_buffSize += pushAmount;
  this->_totalBytesRead += pushAmount;
  return ;
}

/*
**  pushAmount functionality is needed because there are parts of
**  the code, like in FileHandler's writefile method, where there is a need
**  to add as much input content as possible in each call, and it doesn't
**  matter if the size of input is bigger than the buffer size, as the contents
**  of input that are not moved to the buffer stay available in subsequent
**  calls.
**
**  In other situations where all of input's content must be moved to the
**  buffer, the caller must check the available buffer size to ensure
**  the entire input content will fit in the buffer.
*/

void  InputOutput::pushBack(std::string const & input)
{
  std::size_t spaceAvailable;
  std::size_t pushAmount;

  if (this->_buffSize == InputOutput::buffCapacity)
    return ;
  spaceAvailable = this->getAvailableBufferSize();
  pushAmount = input.size() <= spaceAvailable ? input.size() : spaceAvailable;
  this->_buff.replace(this->_buffSize, pushAmount, input, 0, pushAmount);
  this->_buffSize += pushAmount;
  this->_totalBytesRead += pushAmount;
  return ;
}

/*
**  Not applying a real insert because it would increase the real buffer size
**  of _buff.
**  Using replace instead, to preserve the fixed buffer size of 8K.
*/
void  InputOutput::insert(std::size_t insertPos, std::string const & input,
                          std::size_t inputLen)
{
  std::size_t const backStringSize = this->_buffSize - insertPos;

  if (this->_buffSize + inputLen > InputOutput::buffCapacity)
    return ;
  if (insertPos < this->_buffSize)
  {
    //Moving back existing content that will be after the inserted input
    this->_buff.replace(insertPos, backStringSize, &this->_buff[insertPos],
                        0, inputLen);
  }
  //Replace the value of middle bytes by input value
  this->_buff.replace(insertPos, inputLen, input, 0, inputLen);
  this->_buffSize += inputLen;
  this->_totalBytesRead += inputLen;
  return ;
}

std::size_t InputOutput::getBufferSize(void) const
{
  return (this->_buffSize);
}

std::size_t InputOutput::getAvailableBufferSize(void) const
{
  return (InputOutput::buffCapacity - this->_buffSize);
}

void  InputOutput::addBytesRead(std::size_t const bytes)
{
  this->_totalBytesRead += bytes;
  this->_buffSize += bytes;
  return ;
}

void  InputOutput::addBytesSent(std::size_t const bytes)
{
  if (bytes > this->_buffSize)
  {
    std::cout << "ERROR: addBytesSent from IputOutput "
              << "received more bytes sent than buffer size."
              << std::endl;
    return ;
  }
  // Copy remaining buff bytes to the front
  this->_buff.replace(0, this->_buffSize - bytes, &this->_buff.at(bytes));
  // Zero the duplicated bytes at the back
  this->_buff.replace(this->_buffSize - bytes, bytes, bytes, '\0');
  this->_totalBytesSent += bytes;
  this->_buffSize -= bytes;
  return ;
}

std::size_t InputOutput::getPayloadSize(void) const
{
  return (this->_payloadSize);
}

void  InputOutput::setPayloadSize(std::size_t const size)
{
  this->_payloadSize = size;
  return ;
}

bool  InputOutput::finishedSend(void) const
{
  return (this->_payloadSize == this->_totalBytesSent);
}

bool  InputOutput::finishedRead(void) const
{
  return (this->_payloadSize == this->_totalBytesRead);
}

void  InputOutput::setFinishedSend(void)
{
  this->_payloadSize = this->_totalBytesSent;
}

void  InputOutput::setFinishedRead(void)
{
  this->_payloadSize = this->_totalBytesRead;
}

bool  InputOutput::isFirstRead(void) const
{
  return (this->_totalBytesRead == 0
          || this->_totalBytesRead == this->_buffSize);
}

void  InputOutput::clear(void)
{
  //replace instead of clear to preserve buffer capacity
  this->_buff.replace(0, this->_buffSize, this->_buffSize, '\0');
  this->_buffSize = 0;
  this->_payloadSize = 0;
  this->_totalBytesRead = 0;
  this->_totalBytesSent = 0;
}
