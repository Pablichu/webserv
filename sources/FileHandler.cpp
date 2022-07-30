#include "FileHandler.hpp"

FileHandler::FileHandler(void) {}

FileHandler::~FileHandler(void) {}

// Open file for reading

bool	FileHandler::openFile(std::string const & filePath, int & fd)
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
	std::stringstream	headers;
	std::size_t				headersSize;
	std::size_t				bytesRead;

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
	headers << Response::protocol
					<< " " << connData.rspStatus << " "
					<< Response::statusCode.find(connData.rspStatus)->second
					<< '\n';
	headers << "Content-length: " << connData.fileSize << '\n';
	headers << "Content-type: "
					<< Response::contentType.find(
							utils::getFileExtension(connData.filePath)
							)->second + "; charset=utf-8"
					<< "\n\n";
	headersSize = headers.str().size();
	connData.rspBuff.replace(0, headersSize, headers.str());
	bytesRead = read(fd, &connData.rspBuff[headersSize],
													ConnectionData::rspBuffCapacity - headersSize);
	if (bytesRead <= 0)
	{
		close(fd);
		return (false);
	}
	connData.rspBuffSize = bytesRead + headersSize;
	connData.totalBytesRead = bytesRead;
	connData.rspSize = headersSize + connData.fileSize;
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
	bytesRead = read(fd, &connData.rspBuff[endContent],
													ConnectionData::rspBuffCapacity - endContent);
	if (bytesRead <= 0)
		return (false);
	connData.rspBuffSize = endContent + bytesRead;
	connData.totalBytesRead += bytesRead;
	return (true);
}
