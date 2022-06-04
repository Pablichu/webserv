#include "webserv.hpp"
#include <assert.h>

void	openFile(void)
{
	int				fd;
	Response	res;

	assert(res.openFile("non_existent", fd) == false);
	assert(fd == -1);
	assert(res.openFile("tests/response.cpp", fd) == true);
	assert(fd > 2 && fd < 256);
	assert(close(fd) == 0);
	return ;
}

void	readSmallFile(void)
{
	std::ofstream	ostr;
	int						fd;
	std::string		filePath;
	std::string		rsp;
	std::size_t		totalBytesRead;
	long					fileSize;	
	Response			res;

	filePath = getenv("PWD");
	filePath.append("/sampleFile.html");
	ostr.open(filePath.c_str());
	ostr << "<html><head><title>Index</title></head>";
	ostr << "<body><h1>Index</h1></body></html>";
	ostr.close();
	fd = open(filePath.c_str(), O_RDONLY);
	assert(res.readFile(filePath, fd, rsp, totalBytesRead, fileSize) == true);
	assert(rsp.size() > totalBytesRead); //Because headers are added to rsp
	assert(lseek(fd, 0, SEEK_END) == fileSize);
	assert(static_cast<long>(totalBytesRead) == fileSize);
	close(fd);
	remove(filePath.c_str());
	return ;
}

void	readBigFile(void)
{
	std::ofstream	ostr;
	std::vector<char>	v(1000, 'a');
	int						fd;
	std::string		filePath;
	std::string		rsp;
	std::size_t		totalBytesRead;
	long					fileSize;	
	Response			res;

	filePath = getenv("PWD");
	filePath.append("/sampleFile.html");
	ostr.open(filePath.c_str());
	for (std::size_t i = 0; i < 10; ++i)
		ostr.write(&v[0], v.size());
	ostr.close();
	fd = open(filePath.c_str(), O_RDONLY);
	//First readFile call
	assert(res.readFile(filePath, fd, rsp, totalBytesRead, fileSize) == true);
	assert(rsp.size() > totalBytesRead); //Because headers are added to rsp
	assert(10000 == fileSize);
	assert(static_cast<long>(totalBytesRead) < fileSize);
	assert(totalBytesRead == 8192);
	rsp.clear();
	//Second readFile call
	assert(res.readFile(fd, rsp, totalBytesRead) == true);
	assert(static_cast<long>(totalBytesRead) == fileSize);
	assert(rsp.size() && rsp.size() < 8192);
	close(fd);
	remove(filePath.c_str());
	return ;
}

void	sendSmallFile(void)
{
	std::string	rsp;
	std::size_t	rspSize;
	std::size_t	totalBytesSent = 0;
	int					sockPair[2]; //JUST FOR TESTING
	Response		res;

	rsp = "Hola, Mundo!";
	rspSize = rsp.size();
	socketpair(AF_UNIX, SOCK_STREAM, 0, sockPair);
	assert(res.sendFile(sockPair[1], rsp, totalBytesSent) == true);
	assert(totalBytesSent == rspSize);
	close(sockPair[0]);
	close(sockPair[1]);
	return ;
}

void	sendBigFile(void)
{
	std::string	rsp;
	std::size_t	rspSize = 10000;
	std::size_t	totalBytesSent = 0;
	int					sockPair[2]; //JUST FOR TESTING
	Response		res;

	rsp.insert(0, 8192, 'a');
	socketpair(AF_UNIX, SOCK_STREAM, 0, sockPair);
	//First sendFile call
	assert(res.sendFile(sockPair[1], rsp, totalBytesSent) == true);
	assert(totalBytesSent == 8192);
	assert(rsp.size() == 0);
	rsp.insert(0, 10000 - 8192, 'a');
	//Second sendFile call
	assert(res.sendFile(sockPair[1], rsp, totalBytesSent) == true);
	assert(totalBytesSent == rspSize);
	assert(rsp.size() == 0);
	close(sockPair[0]);
	close(sockPair[1]);
	return ;
}

int		main(void)
{
	std::cout << "\n--- RESPONSE TESTS ---\n";
	openFile();
	std::cout << "\nOPEN FILE: OK\n";
	readSmallFile();
	std::cout << "\nREAD SMALL FILE: OK\n";
	readBigFile();
	std::cout << "\nREAD BIG FILE: OK\n";
	sendSmallFile();
	std::cout << "\nSEND SMALL FILE: OK\n";
	sendBigFile();
	std::cout << "\nSEND BIG FILE: OK\n";
	std::cout << std::endl;
	return (0);
}
