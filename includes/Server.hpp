#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <poll.h>

#include "webserv.hpp"

#include <map>
#include <algorithm>

struct	ServerConfig;
struct	LocationConfig;

#define MAX_REQUEST 5 //TRY WITH 10?

struct	ConnectionData
{
	std::vector<ServerConfig const *> *	portConfigs;
	std::size_t													serverIndex;
	std::size_t													locationIndex;
	std::string													dataIn;
	std::string													dataOut;
};

class	Server
{
	private:

		std::map<int, std::vector<ServerConfig const *> >		_listeningSockets;
		std::map<int, ConnectionData >											_connectionSockets;
		struct pollfd *																			_connections;
		std::size_t																					_connLen;
		std::size_t																					_connCap;

		bool	_initSocket(int & sock, std::size_t const port);
		void	_removeConnection(std::size_t index);
		bool	_sendData(int socket);
		void	_getResponse(std::string & data, ConnectionData const & conn) const;
		void  _matchLocation(std::vector<LocationConfig> const & servers,
												std::size_t & index, std::string const & reqUri);
		void  _matchServer(std::vector<ServerConfig const *> & servers,
												std::size_t & index, std::string const & reqHost);
		void	_matchConfig(int socket);
		bool	_receiveData(int socket);
		void	_increaseConnCap(void);
		bool	_acceptConn(int	socket);
		bool	_handleEvent(std::size_t index);
		void	_monitorListenSocketEvents(void);

	public:
		Server();
		~Server();

		bool	start(void);
		bool	prepare(std::vector<ServerConfig> const & config);
};
