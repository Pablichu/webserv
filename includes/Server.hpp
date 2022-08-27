#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>
#include <poll.h>

#include <map>
#include <algorithm>

#include "webserv.hpp"

#define MAX_REQUEST 5 //TRY WITH 10?

class	Server
{
	private:

		FdTable			_fdTable;
		Monitor			_monitor;
		Response		_response;

		bool	_initSocket(int & sock, std::size_t const port);
		void	_endConnection(int fd, size_t connIndex);
		void	_sendData(int socket, std::size_t index);
		bool	_fillFileResponse(int const fd, int const index);
		bool	_matchLocation(std::vector<LocationConfig> const & servers,
													std::size_t & index, std::string const & reqUri);
		void  _matchServer(std::vector<ServerConfig const *> & servers,
												std::size_t & index, std::string const & reqHost);
		bool	_matchConfig(int socket);
		bool	_validRequest(int socket, int & error);
		void	_handlePipeRead(int const fd, std::size_t const index);
		void	_handleClientRead(int socket);
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
