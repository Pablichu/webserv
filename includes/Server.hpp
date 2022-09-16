#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>
#include <poll.h>

#include "Monitor.hpp"
#include "FdTable.hpp"
#include "EventHandler.hpp"

#define MAX_REQUEST 5 //TRY WITH 10?

class	Server
{
	private:

		FdTable						_fdTable;
		Monitor						_monitor;
		ConnectionHandler	_connHandler;
		EventHandler			_eventHandler;

		bool	_initSocket(int & sock, std::size_t const port);
		void	_handleEvent(std::size_t index);
		bool	_checkTimeout(int const fd, std::size_t const index);

	public:
		Server();
		~Server();

		bool	start(void);
		bool	prepare(std::vector<ServerConfig> const & config);
};
