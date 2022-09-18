#include "EventHandler.hpp"

EventHandler::EventHandler(Monitor & monitor, FdTable & fdTable,
                            ConnectionHandler & connHandler)
                            : _monitor(monitor), _fdTable(fdTable),
                              _connHandler(connHandler),
                              _response(_fdTable, _monitor)
{}

EventHandler::~EventHandler(void)
{}

/*
**  Accept client connections from any virtual server listening socket
**  and add them to the connections array that is passed to poll.
*/

void  EventHandler::connectionAccept(int const listenSocket) 
{
  this->_connHandler.accept(listenSocket);
}

void  EventHandler::connectionRead(int const fd, std::size_t const index)
{
  ConnectionData &  connData = this->_fdTable.getConnSock(fd);

  if (connData.status == Idle)
    connData.status = Active;
  // Connected client socket is ready to read without blocking
  if (!this->_connHandler.receive(fd))
  {
    this->_connHandler.end(fd);
    return ;
  }
  if (connData.req.getDataSate() != done
      && connData.req.dataAvailible())
    this->_processConnectionRead(this->_monitor[index]);
}

void  EventHandler::connectionWrite(int const fd, std::size_t const index)
{
  ConnectionData &  connData = this->_fdTable.getConnSock(fd);

  if (connData.io.getBufferSize())
    this->_connHandler.send(fd);
  if (connData.dirListNeedle)
  { // build next directory listing chunk
    this->_response.buildDirList(this->_monitor[index], connData);
  }
  //This check is relevant, because the entire buffer might not be sent.
  if(connData.io.getBufferSize() == 0)
    this->_monitor[index].events = POLLIN;
  if (connData.io.finishedSend()
      && connData.io.getPayloadSize() != 0)
  { // Request handling finished succesfully
    if (connData.req.getPetit("CONNECTION") != "close"
        && connData.handledRequests < ConnectionData::max - 1)
      connData.setIdle();
    else
      this->_connHandler.end(fd);
  }
}

void  EventHandler::pipeWrite(int const fd)
{
  CgiData &         cgiData = this->_fdTable.getPipe(fd);
  ConnectionData &  connData = this->_fdTable.getConnSock(cgiData.socket->fd);
  int               readPipeFd;

  if (!this->_response.cgiHandler.sendBody(fd, connData))
    { // Order of statements is important!!
      std::cout << "sendBody to PipeWrite failed." << std::endl;
      connData.io.clear();
      // Build Internal Server Error
      this->_response.sendError(*connData.cgiData->socket, 500);
      readPipeFd = connData.cgiData->getROutPipe();
      this->_monitor.remove(fd);
      this->_monitor.remove(readPipeFd);
      // Remove cgiData from ConnectionData, closing both pipes.
      connData.cgiData->closePipes();
      connData.cgiData = 0;
      this->_fdTable.remove(fd);
      this->_fdTable.remove(readPipeFd);
      return ;
    }
    if (connData.io.finishedSend())
    {
      /*
      **  Close pipe fd, but do not delete CgiData structure,
      **  only PipeRead type must do it, because it is shared between the two.
      */
      connData.io.clear();
      this->_monitor.remove(fd);
	    this->_fdTable.remove(fd);
      //CgiData has not been deleted yet, because the Read Pipe is still open
      connData.cgiData->closeWInPipe();
    }
}

void  EventHandler::pipeRead(int const fd)
{
  pollfd &          socket = *this->_fdTable.getPipe(fd).socket;
  ConnectionData &  connData = this->_fdTable.getConnSock(socket.fd);
  int               error = 0;
  int               exitStatus;

  exitStatus = this->_response.cgiHandler.getExitStatus(connData.cgiData->pID);
  if (exitStatus < 0
      || !this->_response.cgiHandler.receiveData(fd, connData))
  {
    std::cout << "Pipe read failed." << std::endl;
    if (!exitStatus)
      this->_response.cgiHandler.terminateProcess(connData.cgiData->pID);
    connData.io.clear();
    // Build Internal Server Error
    this->_response.sendError(*connData.cgiData->socket, 500);
    connData.cgiData->closeROutPipe();
    connData.cgiData = 0;
    this->_monitor.remove(fd);
    this->_fdTable.remove(fd);
    return ;
  }
  if (!(socket.events & POLLOUT) && connData.io.getBufferSize())
    socket.events = POLLIN | POLLOUT;
  if (connData.io.finishedRead())
  { //All data was received
    if (!exitStatus)
      this->_response.cgiHandler.terminateProcess(connData.cgiData->pID);
    connData.cgiData->closeROutPipe();
    //Order of removals and close is important!!!
    connData.cgiData = 0;
    this->_monitor.remove(fd);
    this->_fdTable.remove(fd);
    if (connData.io.getPayloadSize() == 0)
    {
      if (!this->_response.process(socket, error))
        this->_response.sendError(socket, error);
    }
  }
  return ;
}

void  EventHandler::fileRead(int const fd)
{
  FileData &        fileData = this->_fdTable.getFile(fd);
  ConnectionData &  connData = this->_fdTable.getConnSock(fileData.socket->fd);
  InputOutput &     io = connData.io;

  if (!this->_response.fileHandler.readFile(fd, connData))
  {
    std::cout << "Error reading file fd: " << fd << std::endl;
    io.clear();
    // Build Internal Server Error
    this->_response.sendError(*fileData.socket, 500);
    // Delete fileData
    connData.fileData = 0;
    this->_monitor.remove(fd);
    this->_fdTable.remove(fd);
    // File fd already closed by fileHandler on failure
    return ;
  }
  if (!(fileData.socket->events & POLLOUT))
    fileData.socket->events = POLLIN | POLLOUT;
  if (io.finishedRead())
  {
    this->_monitor.remove(fd);
	  this->_fdTable.remove(fd);
    close(fd);
    connData.fileData = 0;
  }
}

void  EventHandler::fileWrite(int const fd)
{
  FileData &        fileData = this->_fdTable.getFile(fd);
  ConnectionData &  connData = this->_fdTable.getConnSock(fileData.socket->fd);

  if (!this->_response.fileHandler.writeFile(fd, connData))
  {// Order of statements is important!!
    connData.io.clear();
    // Build Internal Server Error
    this->_response.sendError(*fileData.socket, 500);
    // Delete fileData
    connData.fileData = 0;
    this->_monitor.remove(fd);
    this->_fdTable.remove(fd);
    // File fd already closed by writeFile on failure
    return ;
  }
  if (connData.io.finishedSend())
  {
    //Order of statements is important!!
    connData.io.clear();
    this->_response.buildUploaded(*fileData.socket, connData,
                                  connData.req.getPetit("PATH"));
    connData.fileData = 0;
    this->_monitor.remove(fd);
    this->_fdTable.remove(fd);
    close(fd);
  }
}

void  EventHandler::_processConnectionRead(pollfd & socket)
{
  ConnectionData &  connData = this->_fdTable.getConnSock(socket.fd);
  int               error = 0;

  if (connData.req.processWhat())
  {
    this->_response.sendError(socket, 413);
    return ;
  }
  if (connData.req.getDataSate() != done)
    return ;
  connData.io.clear();
  UrlParser().parse(connData.req.getPetit("PATH"),
                    connData.urlData);
  std::cout << "Data received for "
            << connData.req.getPetit("HOST")
            << " with path "
            << connData.req.getPetit("PATH")
            << " | Buffer reached: "
			<< connData.req.updateLoop(false)
			<< std::endl;
  if (!this->_validRequest(socket.fd, error)
      || !this->_response.process(socket, error))
    this->_response.sendError(socket, error);
}

bool  EventHandler::_validRequest(int socket, int & error)
{
  ConnectionData &  connData = this->_fdTable.getConnSock(socket);

  if (!this->_configMatcher.match(connData))
  { //Request path did not match any server's location uri
    error = 404; // Not Found
    return (false);
  }
  if (!connData.getLocation()->methods.count(connData.req.getPetit("METHOD")))
  { //Request method is not allowed in target location
    error = 405; // Method Not Allowed
  }
  //else if (!validFileExtension)
    //error = 415; // Unsupported Media Type
  //else if (!validStandardHeaders)
    //error = 400; // Bad Request
  else if (connData.req.getPetit("PROTOCOL") != "HTTP/1.1")
    error = 505; // HTTP Version Not Supported
  if (error)
    return (false);
  return (true);
}
