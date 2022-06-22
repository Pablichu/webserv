#include "Response.hpp"

std::string const Response::protocol = "HTTP/1.1";

Response::Response(void) {}

Response::~Response(void) {}

// Open file

bool	Response::openFile(std::string const & filePath, int & fd)
{
	/*
	**	NOT SETTING THIS fd AS NON-BLOCKING, BECAUSE IT HAS NO EFFECT
	**	ON REGULAR FILES.
	*/
	fd = open(filePath.c_str(), O_RDONLY);
	if (fd == -1)
		return (false);
	return (true);
}

/*
**	Read file for the first time, adding headers before file content in rsp.
**
**	Provisional
*/

bool	Response::readFileFirst(int const fd, ConnectionData & connData)
{
	std::stringstream	headers;
	std::size_t				headersSize;

	status = 200;
	if (connData.filePath.find("404") != std::string::npos)
		status = 404;
	status_msg = "OK";
	if (status != 200)
		status_msg = "Not Found";

	std::string		doctype
	(
		connData.filePath.substr
		(
			connData.filePath.find_last_of('.') + 1,
			connData.filePath.size() - connData.filePath.find_last_of('.') + 1
		)
	);
	cnt_type = "text/" + doctype + "; charset=utf-8"; //TODO: cambiar en futuro
	connData.fileSize = static_cast<long>(lseek(fd, 0, SEEK_END));
	if (connData.fileSize == -1)
	{
		close(fd);
		return (false);
	}
	if (lseek(fd, 0, SEEK_SET) == -1)
	{
		close(fd);
		return (false);
	}
	headers << protocol << " " << status << " " << status_msg << '\n';
	headers << "Content-length: " << connData.fileSize << '\n';
	headers << "Content-type: " << cnt_type << "\n\n";
	headersSize = headers.str().size();
	connData.rspBuff.replace(0, headersSize, headers.str());
	this->bytesRead = read(fd, &connData.rspBuff[headersSize],
													ConnectionData::rspBuffCapacity - headersSize);
	if (this->bytesRead <= 0)
	{
		close(fd);
		return (false);
	}
	connData.rspBuffSize = this->bytesRead + headersSize;
	connData.totalBytesRead = this->bytesRead;
	connData.rspSize = headersSize + connData.fileSize;
	return (true);
}

/*
**	Subsequent file reads. This function gets called if read buffer size
**	is less than file size.
**
**	The fd is closed outside when returns false or file was read completely.
*/

bool	Response::readFileNext(int const fd, ConnectionData & connData)
{
	std::size_t	endContent;

	endContent = connData.rspBuffSize;
	if (connData.rspBuffOffset)
	{
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
	this->bytesRead = read(fd, &connData.rspBuff[endContent],
													ConnectionData::rspBuffCapacity - endContent);
	if (bytesRead <= 0)
		return (false);
	connData.rspBuffSize = endContent + this->bytesRead;
	connData.totalBytesRead += this->bytesRead;
	return (true);
}

/*
**	Sends read file content to client socket. It may be called more than once
**	if file size is bigger than read buffer size.
*/

bool	Response::sendFile(int const sockFd, ConnectionData & connData)
{
	this->bytesSent = send(sockFd, &connData.rspBuff[0], connData.rspBuffSize, 0);
	if (this->bytesSent <= 0)
	{
		std::cout << "Could not send data to client." << std::endl;
		return (false);
	}
	connData.totalBytesSent += this->bytesSent;
	if (this->bytesSent == connData.rspBuffSize)
	{
		connData.rspBuff.replace(0, connData.rspBuffSize, 1, '\0');
		connData.rspBuffSize = 0;
	}
	else
		connData.rspBuffOffset = this->bytesRead;
	return (true);
}
