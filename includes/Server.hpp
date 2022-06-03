#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <poll.h>
#include <dirent.h>

#include <map>
#include <algorithm>

#include "webserv.hpp"

struct	ServerConfig;
struct	LocationConfig;

class		CgiResponse;

#define MAX_REQUEST 5 //TRY WITH 10?

// One ConnectionData per client (connection) socket

struct	ConnectionData
{
	std::vector<ServerConfig const *> *	portConfigs;
	std::size_t													serverIndex;
	std::size_t													locationIndex;
	Request															req;
	int																	fileFd;
	std::string													filePath;
	long																fileSize;
	std::size_t													totalBytesRead;
	std::size_t													totalBytesSent;
	std::size_t													rspSize;
	std::string													rsp;

	ConnectionData(void);
	~ConnectionData(void);

	ServerConfig const *		getServer(void);
	LocationConfig					getLocation(void);
};

class	Server
{
	private:

		std::map<int, std::vector<ServerConfig const *> >		_listeningSockets;
		std::map<int, ConnectionData >											_connectionSockets;
		std::map<int, std::pair< int, std::size_t> >				_fileFds;
		std::map<int, CgiResponse *>												_cgiPipes;
		Monitor																							_monitor;
		Response																						_response;

		bool	_initSocket(int & sock, std::size_t const port);
		void	_endConnection(int fd, size_t connIndex);
		void	_sendData(int socket, std::size_t index);
		bool	_launchCgi(int socket, ConnectionData & conn,
											std::size_t connIndex);
		bool	_fillFileResponse(int const fd, int const index);
		std::string	_listDir(std::string const & uri,
                          std::string const & root) const;
		void	_getFilePath(ConnectionData & conn) const;
		void  _matchLocation(std::vector<LocationConfig> const & servers,
												std::size_t & index, std::string const & reqUri);
		void  _matchServer(std::vector<ServerConfig const *> & servers,
												std::size_t & index, std::string const & reqHost);
		void	_matchConfig(int socket);
		bool  _prepareResponse(int socket, std::size_t index);
		bool	_receiveCgiData(int rPipe);
		bool	_receiveData(int socket);
		void	_acceptConn(int	socket);
		void	_handleEvent(std::size_t index);
		void	_monitorListenSocketEvents(void);

	public:
		Server();
		~Server();

		bool	start(void);
		bool	prepare(std::vector<ServerConfig> const & config);
};
