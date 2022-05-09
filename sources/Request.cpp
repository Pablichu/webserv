#include "Request.hpp"

Request::Request(std::string header)
{
	std::string	buff;
	size_t		pos;
	size_t		rpos;

	//First line
	pos = header.find(" ");
	this->_values["Method"] = header.substr(0, pos);
	pos++;
	rpos = header.find("H") - 1;
	this->_values["Path"] = header.substr(pos, rpos - pos);
	pos = rpos + 1;
	rpos = header.find("\n");
	this->_values["Protocol"] = header.substr(pos, rpos - pos);

	//Get rest of header
	while(true)
	{
		header.erase(0, rpos + 1);
		pos = header.find(":");
		buff = header.substr(0, pos);
		pos += 2;
		rpos = header.find("\n");
		if (rpos == std::string::npos)
		{
			rpos = header.length();
			this->_values[buff] = header.substr(pos, rpos - pos);
			break;
		}
		this->_values[buff] = header.substr(pos, rpos - pos);
	}
}

Request::~Request() {}

const std::string Request::GetPetit(std::string petition)
{
	return this->_values[petition];
}

std::map<std::string, std::string>	Request::GetWholePetit()
{
	return this->_values;
}
