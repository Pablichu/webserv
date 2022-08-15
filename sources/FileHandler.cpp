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

/*
**	Read file for the first time, adding headers before file content in rsp.
**
**	Provisional
*/

bool	FileHandler::readFileFirst(int const fd, ConnectionData & connData)
{
	FileData &				fileData = *(connData.fileData);
	std::stringstream	headers;
	std::size_t				headersSize;
	std::size_t				bytesRead;

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
	headers << HttpInfo::protocol
					<< " " << connData.rspStatus << " "
					<< HttpInfo::statusCode.find(connData.rspStatus)->second
					<< "\r\n";
	headers << "Date: " << utils::getDate() << "\r\n";
	headers << "Content-length: " << fileData.fileSize << "\r\n";
	headers << "Content-type: "
					<< HttpInfo::contentType.find(
							utils::getFileExtension(fileData.filePath)
							)->second + "; charset=utf-8"
					<< "\r\n\r\n";
	headersSize = headers.str().size();
	connData.buff.replace(0, headersSize, headers.str());
	bytesRead = read(fd, &connData.buff[headersSize],
													ConnectionData::buffCapacity - headersSize);
	if (bytesRead <= 0)
	{
		close(fd);
		return (false);
	}
	connData.buffSize = bytesRead + headersSize;
	connData.totalBytesRead = bytesRead;
	connData.rspSize = headersSize + fileData.fileSize;
	return (true);
}

/*
**	Subsequent file reads. This function gets called if read buffer size
**	is less than file size.
**
**	The fd is closed outside when returns false or file was read completely.
*/

bool	FileHandler::readFileNext(int const fd, ConnectionData & connData)
{
	std::size_t	endContent;
	std::size_t	bytesRead;

	endContent = connData.buffSize;
	if (connData.buffOffset)
	{
		//Move the content after offset to the front
		connData.buff.replace(connData.buff.begin(),
			connData.buff.begin() + connData.buffOffset,
			&connData.buff[connData.buffOffset]);
		//Fill the content that is duplicated at the back with NULL
		connData.buff.replace(endContent - connData.buffOffset,
			connData.buffOffset, 0);
		//Update endContent
		endContent = endContent - connData.buffOffset;
		//Reset offset
		connData.buffOffset = 0;
	}
	bytesRead = read(fd, &connData.buff[endContent],
													ConnectionData::buffCapacity - endContent);
	if (bytesRead <= 0)
		return (false);
	connData.buffSize = endContent + bytesRead;
	connData.totalBytesRead += bytesRead;
	return (true);
}

bool	FileHandler::writeFile(int const fd, ConnectionData & connData) const
{
	std::string const &	body = (connData.req.getHeaders())["Body"];
	int									len;
	size_t							remainingBytes;
	size_t							sendBytes;

	remainingBytes = body.length() - connData.totalBytesSent;
	sendBytes = remainingBytes <= ConnectionData::buffCapacity
							? remainingBytes : ConnectionData::buffCapacity;
	len = write(fd, &body[connData.totalBytesSent], sendBytes);
	if (len <= 0)
		return (false);
	connData.totalBytesSent += len;
	return (true);
}
