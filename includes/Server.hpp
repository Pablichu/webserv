#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <poll.h>

#include "webserv.hpp"

#include <vector> // quitar
#include <algorithm>

#define PORT 8080
#define MAX_REQUEST 5 //TRY WITH 10?

class	Server
{
	private:

		std::vector<int>		_sockets;
		struct sockaddr_in	_address;
		int									_addrlen;
		struct pollfd *			_connections;
		std::size_t					_connLen;
		std::size_t					_connCap;

		void	_removeConnection(std::size_t index);
		bool	_receiveData(int socket, std::string & data);
		void	_increaseConnCap(void);
		bool	_acceptConn(int	socket);

	public:
		Server();
		~Server();

		bool	start(void);
		bool	prepare(void);
};
