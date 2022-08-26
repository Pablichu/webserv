#include "Request.hpp"

Request::Request() : _loops(0), _type(none), _length(0) {}

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
	/*std::map<std::string, std::string>::iterator  it;
	for (it = this->_values.begin(); it != this->_values.end(); it++) {
		std::cout << it->first << " = " << it->second << std::endl;
	}*/
	if (this->getPetit("Transfer-Encoding") == "chunked")//Check, maybe some parts are not neccessary
	{
		this->_type = chunked;
		processChunked();
	}
	else if (this->getPetit("Content-Length") != "")
	{
		std::cout << "body process" << std::endl;
		this->_type = normal;
		this->_length = this->_stoi_mine(this->getPetit("Content-Length"));
		std::cout << "Length > " << this->_length << std::endl;
		this->processBody();
	}
	else
		this->_type = done;
}

void	Request::processChunked()
{
	std::string &	body = this->_values["Body"];
	std::string &	data = this->_data;
	int				pos;
	int				buffer;

	while (!data.empty())
	{
		if (!data.find("0\r\n\r\n"))
		{
			this->_type = done;
			data.clear();
			std::cout << "Mensaje de felicidad extrema" << std::endl;
			return ;
		}

		pos = data.find("\r\n");
		if (pos == -1)
			return ;
		buffer = _hextodec(data.substr(0, pos));
		//std::cout << " >> " << pos << "/" << data.substr(0, pos) << "/" << buffer << std::endl;
		body.append(data.substr(pos + 2, buffer));
		data.erase(0, pos + buffer + 4);//this 4 is from two "\r\n"
	}
}

void	Request::processBody()
{
	std::string &reqData = this->_data;

	/*if (!reqData.size())
	{
		std::cout << " >>>> No body to read" << std::endl;
		exit(0);
	}*/
	if (reqData.size() > this->_length)
	{
		std::cout << " >>>> Body is too large " << reqData.size() << "/" << this->_length << std::endl;
		exit(0);
	}
	this->_length -= reqData.size();
	this->_values["Body"].append(reqData.substr());
	if (!this->_length)
		this->_type = done;
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

enum bodyType &	Request::getDataSate(void)
{
	return this->_type;
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

size_t	Request::_hextodec(std::string hex)
{
	std::stringstream	ss;
	size_t				nb;

	ss << std::hex << hex;
	ss >> nb;
	return nb;
}

size_t	Request::_stoi_mine(std::string nb)
{
	std::istringstream	is(nb);
	size_t				realNb;
	if (is >> realNb)
		return realNb;
	return 0;
}
