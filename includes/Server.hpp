#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>
#include <poll.h>
#include <dirent.h>

#include <map>
#include <algorithm>

#include "webserv.hpp"

#define MAX_REQUEST 5 //TRY WITH 10?

/*enum		FdType
{
	Socket,
	File,
	Pipe
}				fdType;*/

class	Server
{
	private:

		std::map<int, std::vector<ServerConfig const *> >		_listeningSockets;
		std::map<int, ConnectionData >											_connectionSockets;
		std::map<int, std::pair< int, std::size_t> >				_fileFds;
		std::map<int, CgiData *>														_cgiPipes;
		Monitor																							_monitor;
		Response																						_response;
		CgiHandler																					_cgiHandler;

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
