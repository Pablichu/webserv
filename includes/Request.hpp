#pragma once

#include "webserv.hpp"

class	Request
{
	private:
		//Start line
		std::string	_method;
		std::string	_path;
		std::string	_protocol;

		//Host
		std::string	_host;

		//User-agent --necesary?
		//Content-lenght
		int		_contLenght;

		//Header, check what we need

	public:
		Request(std::string header);
		~Request();

		std::string	GetMethod() const;
		std::string	GetPath() const;
		std::string	GetProtocol() const;
		std::string	GetHost() const;
};
