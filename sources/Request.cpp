#include "Request.hpp"

Request::Request(std::string header)
{
	std::string	buff;
	int			pos;
	int			rpos;
	//First line
	pos = header.find(" ");
	this->_method = header.substr(0, pos);
	pos++;
	rpos = header.find("H") - 1;
	this->_path = header.substr(pos, rpos - pos);
	pos = rpos + 1;
	rpos = header.find("\n");
	this->_protocol = header.substr(pos, rpos - pos);

	//Second line host
	header.erase(0, rpos + 1);
	pos = header.find(" ");
	rpos = header.find("\n") - 1;
	this->_host = header.substr(pos + 1, rpos - pos);
}

Request::~Request() {}

std::string	Request::GetMethod() const
{
	return this->_method;
}

std::string	Request::GetPath() const
{
	return this->_path;
}

std::string	Request::GetProtocol() const
{
	return this->_protocol;
}

std::string	Request::GetHost() const
{
	return this->_host;
}
