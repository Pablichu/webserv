#include "CgiHandler.hpp"

CgiHandler::CgiHandler(void)
{
  return ;
}

CgiHandler::~CgiHandler(void)
{
  return ;
}

/*
**  Receive data from a cgi pipe's read end.
**
**  Need to check Content-length header from cgi response to know when
**  all the data from cgi program has been read.
*/

bool  CgiHandler::receiveData(int rPipe, ConnectionData & connData)
{
  std::size_t	endContent;
  int         len;

  endContent = connData.rspBuffSize;
	if (connData.rspBuffOffset)
	{ //This portion of code is equal to the one in Response::readFileNext
		//Move the content after offset to the front
		connData.rspBuff.replace(connData.rspBuff.begin(),
			connData.rspBuff.begin() + connData.rspBuffOffset,
			&connData.rspBuff[connData.rspBuffOffset]);
		//Fill the content that is duplicated at the back with NULL
		connData.rspBuff.replace(endContent - connData.rspBuffOffset,
			connData.rspBuffOffset, 0);
		//Update endContent
		endContent = endContent - connData.rspBuffOffset;
		//Reset offset
		connData.rspBuffOffset = 0;
	}
  len = read(rPipe, &connData.rspBuff[endContent],
													ConnectionData::rspBuffCapacity - endContent);
  if (len == 0)
  { //EOF
    std::cout << "Pipe write end closed. No more data to read." << std::endl;
    if (!connData.rspSize)
      connData.rspSize = connData.totalBytesRead;
    return (true);
  }
  // An error occurred. EAGAIN is not possible, because POLLIN was emitted.
  if (len < 0)
    return (false);
  if (!connData.totalBytesRead) //First call to receiveData
  {
    /*
    **  Provisional. rspSize must be obtained by parsing the Content-length
    **  header sent by cgi program
    */
    /*
    **  Here there should be a call to another method that parses
    **  the cgi response headers
    */
    connData.rspSize = 0; //Indicates unknown size
  }
  connData.rspBuffSize = endContent + len;
	connData.totalBytesRead += len;
  return (true);
}

// Provisional

void  CgiHandler::_execProgram(CgiData const & cgiData,
                                ConnectionData & connData)
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
  dup2(cgiData.inPipe[0], STDIN_FILENO);
  close(cgiData.inPipe[0]);
  dup2(cgiData.outPipe[1], STDOUT_FILENO);
  close(cgiData.outPipe[1]);
  execve(programPath.c_str(), argv, NULL); //Third argument must be the cgi env variables
  delete [] argv;
}

bool  CgiHandler::initPipes(CgiData & cgiData, ConnectionData & connData)
{
  pid_t child;

  if (pipe(cgiData.inPipe) == -1
      || pipe(cgiData.outPipe) == -1)
    return (false);
  child = fork();
  if (child < 0)
    return (false);
  if (!child)
  { //Child process
    close(cgiData.inPipe[1]);
    close(cgiData.outPipe[0]);
    this->_execProgram(cgiData, connData);
    std::cout << "exec failed" << std::endl;
    exit(EXIT_FAILURE);
  }
  else
  { //Parent process
    close(cgiData.inPipe[0]);
    close(cgiData.outPipe[1]);
    return (true);
  }
}
