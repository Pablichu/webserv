#include "FileHandler.hpp"

FileHandler::FileHandler(void) {}

FileHandler::~FileHandler(void) {}

// Open file for reading

bool	FileHandler::openFile(std::string const & filePath, int & fd,
														int const flags, int const mode)
{
	/*
	**	NOT SETTING THIS fd AS NON-BLOCKING, BECAUSE IT HAS NO EFFECT
	**	ON REGULAR FILES.
	*/
	fd = open(filePath.c_str(), flags, mode);
	if (fd == -1)
		return (false);
	return (true);
}

/*
**	If no processes have the file open, it is deleted. Otherwise,
**	it will not be deleted until the last file descriptor
**	referring to it is closed.
**
**	remove returns -1 on failure, 0 otherwise.
*/

bool	FileHandler::removeFile(std::string const & filePath) const
{
	if (remove(filePath.c_str()))
		return (false);
	return (true);
}

bool	FileHandler::readFile(int const fd, ConnectionData & connData)
{
	InputOutput &	io = connData.io;

	if (io.isFirstRead())
	{
		if (!this->_readFileFirst(fd, connData))
			return (false);
	}
	else
	{
		if (!this->_readFileNext(fd, connData))
			return (false);
	}
	return (true);
}

bool	FileHandler::writeFile(int const fd, ConnectionData & connData) const
{
	int						len;
	InputOutput &	io = connData.io;
	std::string &	body = (connData.req.getHeaders())["BODY"];

	if (io.getPayloadSize() == 0)
		io.setPayloadSize(body.size());
	io.pushBack(body);
	len = write(fd, io.outputBuffer(), io.getBufferSize());
	if (len <= 0)
	{
		close(fd);
		// No need to check return value. If it does not exist it won't be removed.
		this->removeFile(connData.fileData->filePath);
		return (false);
	}
	io.addBytesSent(len);
	return (true);
}

/*
**	Read file for the first time, adding headers before file content in rsp.
*/

bool	FileHandler::_readFileFirst(int const fd, ConnectionData & connData)
{
	FileData &		fileData = *(connData.fileData);
	InputOutput &	io = connData.io;
	std::string		headers;
	std::size_t		bytesRead;

	fileData.fileSize = static_cast<long>(lseek(fd, 0, SEEK_END));
	if (fileData.fileSize == -1)
	{
		close(fd);
		return (false);
	}
	if (lseek(fd, 0, SEEK_SET) == -1)
	{
		close(fd);
		return (false);
	}
	headers = HttpInfo::protocol + ' ';
	headers += utils::toString(fileData.rspStatus) + ' ';
	headers += HttpInfo::statusCode.find(fileData.rspStatus)->second + "\r\n";
	headers += "Date: " + utils::getDate() + "\r\n";
	utils::addKeepAliveHeaders(headers, connData.handledRequests,
								connData.req.getPetit("CONNECTION") == "close");
	headers += "Content-length: " + utils::toString(fileData.fileSize) + "\r\n";
	headers += "Content-type: " + HttpInfo::contentType.find(
								utils::getFileExtension(fileData.filePath)
							)->second + "; charset=utf-8" + "\r\n";
	headers += "\r\n";
	io.pushBack(headers);
	io.setPayloadSize(io.getBufferSize() + fileData.fileSize);
	bytesRead = read(fd, io.inputBuffer(), io.getAvailableBufferSize());
	if (bytesRead <= 0)
	{
		close(fd);
		return (false);
	}
	io.addBytesRead(bytesRead);
	return (true);
}

/*
**	Subsequent file reads. This function gets called if read buffer size
**	is less than file size.
*/

bool	FileHandler::_readFileNext(int const fd, ConnectionData & connData)
{
	std::size_t		bytesRead;
	InputOutput &	io = connData.io;

	if (io.getBufferSize() == InputOutput::buffCapacity)
		return (true);
	bytesRead = read(fd, io.inputBuffer(), io.getAvailableBufferSize());
	if (bytesRead <= 0)
	{
		close(fd);
		return (false);
	}
	io.addBytesRead(bytesRead);
	return (true);
}
