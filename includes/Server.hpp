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

class	Server
{
	private:

		FdTable						_fdTable;
		Monitor						_monitor;
		ConnectionHandler	_connHandler;
		EventHandler			_eventHandler;

		bool	_initSocket(int & sock, std::size_t const port);
		void	_handleEvent(std::size_t index);
		bool	_checkActive(int const fd, ConnectionData & connData);
		bool	_checkIdle(int const fd, time_t const lastActive);
		bool	_checkTimeout(int const fd);

	public:

		static int const	maxRequests;

		Server();
		~Server();

		bool	start(void);
		bool	prepare(std::vector<ServerConfig> const & config);
};
