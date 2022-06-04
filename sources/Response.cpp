#include "Response.hpp"

std::string const Response::protocol = "HTTP/1.1";

Response::Response(void)
{
	std::fill(this->buff, this->buff + this->buffSize + 1, 0);
	return ;
}

Response::~Response(void) {}

// Open file and set non-blocking fd

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

bool	Response::readFile(std::string const & filePath, int const fd,
												std::string	& rsp, std::size_t & totalBytesRead,
												long & fileSize) //Too many args
{
	std::stringstream	content;

	status = 200;
	if (filePath.find("404") != std::string::npos)
		status = 404;
	status_msg = "OK";
	if (status != 200)
		status_msg = "Not Found";

	std::string		doctype
	(
		filePath.substr
		(
			filePath.find_last_of('.') + 1,
			filePath.size() - filePath.find_last_of('.') + 1
		)
	);
	cnt_type = "text/" + doctype + "; charset=utf-8"; //TODO: cambiar en futuro
	fileSize = static_cast<long>(lseek(fd, 0, SEEK_END));
	if (fileSize == -1)
	{
		close(fd);
		return (false);
	}
	if (lseek(fd, 0, SEEK_SET) == -1)
	{
		close(fd);
		return (false);
	}
	content << protocol << " " << status << " " << status_msg << '\n';
	content << "Content-length: " << fileSize << '\n';
	content << "Content-type: " << cnt_type << "\n\n";
	this->bytesRead = read(fd, this->buff, Response::buffSize);
	if (this->bytesRead <= 0)
	{
		close(fd);
		return (false);
	}
	content << this->buff;
	totalBytesRead = this->bytesRead;
	std::fill(buff, buff + buffSize, 0);
	rsp = content.str();
	return (true);
}

/*
**	Subsequent file reads. This function gets called if read buffer size
**	is less than file size.
**
**	The fd is closed outside when returns false or file was read completely.
*/

bool	Response::readFile(int const fd, std::string & rsp,
											std::size_t & totalBytesRead)
{
	this->bytesRead = read(fd, this->buff, Response::buffSize);
	if (bytesRead <= 0)
		return (false);
	/*
	**	Using append instead of =, in case there was something left in rsp
	**	that could not be sent in the previous Server's _sendData call.
	*/
	rsp.append(this->buff);
	totalBytesRead += this->bytesRead;
	std::fill(buff, buff + buffSize, 0);
	return (true);
}

/*
**	Sends read file content to client socket. It may be called more than once
**	if file size is bigger than read buffer size.
*/

bool	Response::sendFile(int const sockFd, std::string & rsp,
													std::size_t & totalBytesSent)
{
	this->bytesSent = send(sockFd, rsp.c_str(), rsp.size(), 0);
	if (this->bytesSent <= 0)
	{
		std::cout << "Could not send data to client." << std::endl;
		return (false);
	}
	totalBytesSent += this->bytesSent;
	/*
	**	Using erase instead of clear in case the total contents of rsp
	**	are not sent, as sockFd is non-blocking.
	*/
	rsp.erase(0, this->bytesSent);
	return (true);
}