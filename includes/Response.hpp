#pragma once

#include "webserv.hpp"

class	Response
{
	private:
		//Status line
		std::string	protocol; //	Usually HTTP/1.1
		int			status;
		std::string	status_msg;

		//Headers
		//std::string	cnt_encoding;
		std::string	cnt_type; //	Usually text/html; charset=utf-8
		int			cnt_length;

		//Body
		std::string	body;

	public:
		Response(std::string file_route);
		virtual ~Response();

		std::string		get();
};
