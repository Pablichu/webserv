#include "Request.hpp"

Request::Request(std::size_t max_body_size) : _loops(0), _type(none), _length(0),
											  _dataAvailible(false), _max_body_size(max_body_size)
{}

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
	this->_values["METHOD"] = reqData.substr(0, pos);
	pos++;
	rpos = reqData.find("H") - 1;
	this->_values["PATH"] = reqData.substr(pos, rpos - pos);
	pos = rpos + 1;
	rpos = reqData.find("\r\n");
	this->_values["PROTOCOL"] = reqData.substr(pos, rpos - pos);

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
		buff = this->stoupper(reqData.substr(0, pos));
		pos += 2;
		rpos = reqData.find("\n");
		if (rpos == std::string::npos)
			break;
		this->_values[buff] = reqData.substr(pos, rpos - pos - 1);
	}
	if (this->getPetit("TRANSFER-ENCODING") == "chunked")
	{
		this->_type = chunked;
		processChunked();
	}
	else if (this->getPetit("CONTENT-LENGTH") != "")
	{
		this->_type = normal;
		this->_length = this->_stoi_mine(this->getPetit("CONTENT-LENGTH"));
		this->processBody();
	}
	else
		this->_type = done;
}

bool	Request::processChunked()
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
			std::cout << "CHUNKED FINISHED, HAPPY UH?" << std::endl;
			return false;
		}

		pos = data.find("\r\n");
		if (pos == -1)
			return false;
		buffer = _hextodec(data.substr(0, pos));
		body.append(data.substr(pos + 2, buffer));
		data.erase(0, pos + buffer + 2);//this 4 is from two "\r\n"
		if (data.find("\r\n"))
		{
			std::cout << " >>>> Chunk is too large " << data.size() << "/" << buffer << std::endl
					  << "(warning -> with chunked system the length respect to the size of the chunk may not be accurate)" << std::endl;
			data.empty();
			return true;
		}
		data.erase(0, 2);
	}
	return false;
}

bool	Request::processBody()//posible error hablarlo con el compa
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
		return true;
	}
	this->_length -= reqData.size();
	this->_values["Body"].append(reqData.substr());
	if (!this->_length)
		this->_type = done;
	return false;
}

const std::string Request::getPetit(std::string petition)
{
	if (this->_values.count(petition))
		return this->_values[petition];
	return ("");
}

bool		Request::processWhat()
{
	if (this->getDataSate() == normal)
		return this->processBody();
	else if (this->getDataSate() == chunked)
		return this->processChunked();
	else
		this->process();
	return false;
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

bool &			Request::dataAvailible(void)
{
	return this->_dataAvailible;
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

const std::string	Request::stoupper(std::string src)
{
	size_t	i = 0;

	while (src[i])
	{
		if (islower(src[i]))
			src[i] = toupper(src[i]);
		i++;
	}
	return src;	
}
