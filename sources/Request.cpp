#include "Request.hpp"

Request::Request() : _dataState(false), _loops(0), _chunked(false) {}

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
		if (reqData[0] == '\r') // Line terminations in HTTP messages are \r\n
		{
			reqData.erase(0, 2);
			if (reqData.size())
				this->_values["Body"] = reqData.substr();
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
	if (this->_values["Transfer-Encoding"] == "chunked")
		processChunked();
	else
		this->_dataState = false;
	std::cout << ">> Data allocated: " << reqData.size() << std::endl;
}

void	Request::processChunked()
{
	std::cout << "  >>>>>  PROCESSING CHUNKED BODY <<<<<" << std::endl;
	std::string &	body = this->_values["Body"];
	std::string &	data = this->_data;
	int				pos;
	int				buffer;
	//int				rpos;
	std::stringstream ss;

	if (!_chunked && body.size())
	{
		this->_data = body.substr();
		body.clear();
	}
	else if (!_chunked && !body.size())
	{
		_chunked = true;
		return ;
	}

	pos = data.find("/r/n");
	//std::cout << data;
	//rpos = data.rfind("/r/n");
    ss << std::hex << data.substr(0, pos - 1);
	std::cout << pos << "/" << data.substr(0, pos - 1) << "/" << ss << std::endl;
	//exit(0);
	std::cout << body << std::endl;
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

std::map<std::string, std::string> &	Request::getHeaders(void)
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

size_t	Request::updateLoop(bool loop)
{
	if (loop)
		this->_loops++;
	return this->_loops;
}

bool		Request::isChunked() const
{
	return this->_chunked;
}
