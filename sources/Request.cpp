#include "Request.hpp"

Request::Request() {}

Request::~Request() {}

/**
*  To avoid generating a copy of reqData, consider passing
*  std::string const & reqData, but to do this, reqData.erase() cannot be used.
*/
void  Request::process()
{
	std::string	buff;
	size_t		pos;
	size_t		rpos;
	std::string &reqData = this->_data;

	//First line
	pos = reqData.find(" ");
	this->_values["Method"] = reqData.substr(0, pos);
	pos++;
	rpos = reqData.find("H") - 1;
	this->_values["Path"] = reqData.substr(pos, rpos - pos);
	pos = this->_values["Path"].rfind("/");
	this->_values["File"] = this->_values["Path"].substr(pos, rpos - pos);
	pos = rpos + 1;
	rpos = reqData.find("\n");
	this->_values["Protocol"] = reqData.substr(pos, rpos - pos);

	//Get rest of reqData
	while(true)
	{
		reqData.erase(0, rpos + 1);
		//This takes the body in case there is
		if (reqData[0] == '\n')
		{
			reqData.erase(0, 1);
			this->_values["Body"] = reqData;
			break ;
		}
		pos = reqData.find(":");
		buff = reqData.substr(0, pos);
		pos += 2;
		rpos = reqData.find("\n");
		if (rpos == std::string::npos)
			break;
		this->_values[buff] = reqData.substr(pos, rpos - pos - 1);
	}
}

const std::string Request::getPetit(std::string petition)
{
  if (this->_values.count(petition))
	  return this->_values[petition];
  return ("");
}

std::map<std::string, std::string>::iterator	Request::begin()
{
	return this->_values.begin();
}

std::map<std::string, std::string>::iterator	Request::end()
{
	return this->_values.end();
}

std::map<std::string, std::string>	Request::getHeaders(void)
{
	return (this->_values);
}

bool &			Request::getDataSate(void)
{
	return this->_dataState;
}

std::string &	Request::getData(void)
{
	return this->_data;
}
