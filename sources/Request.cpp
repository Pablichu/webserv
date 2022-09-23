#include "Request.hpp"

Request::Request() : _loops(0), _type(none), _length(0), _dataAvailible(false) {}

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
	reqData.erase(0, pos + 1);

	pos = reqData.find(" ");
	this->_values["PATH"] = reqData.substr(0, pos);
	reqData.erase(0, pos + 1);
	pos = 0;

	pos = reqData.find("\r\n");
	this->_values["PROTOCOL"] = reqData.substr(0, pos);
	reqData.erase(0, pos + 2);

	//Get rest of reqData
	rpos = 0;
	while(true)
	{
		if (rpos)
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
	std::string &	body = this->_values["BODY"];
	std::string &	data = this->_data;
	size_t			buffer;
	int				pos;

	while (!data.empty())
	{
		if (!data.find("0\r\n\r\n"))
		{
			this->_type = done;
			data.clear();
			break ;
		}

		pos = data.find("\r\n");
		if (pos == -1)
			break ;
		buffer = _hextodec(data.substr(0, pos));
		if ((buffer + pos + 1) > (data.size() - pos -1) || buffer == 0)
			break ;

		body.append(data.substr(pos + 2, buffer));
		data.erase(0, pos + buffer + 2);//this 4 is from two "\r\n"
		if (data.find("\r\n") || body.size() > this->_max_body_size)
		{
			if (body.size() > this->_max_body_size)
				std::cout << " >>>> Exceeded max body size: " << body.size() << "/" << this->_max_body_size << std::endl;
			else
				std::cout << " >>>> Chunk is too large: " << data.size() << "/" << buffer << std::endl
					<< "(warning -> with chunked system the length respect to the size of the chunk may not be accurate)" << std::endl;
			data.empty();
			return true;
		}
		data.erase(0, 2);
	}
	return false;
}

bool	Request::processBody()
{
	std::string &reqData = this->_data;

	if (reqData.size() > this->_length)
	{
		std::cout << " >>>> Body is too large " << reqData.size() << "/" << this->_length << std::endl;
		return true;
	}
	this->_length -= reqData.size();
	this->_values["BODY"].append(reqData.substr());
	if (!this->_length)
		this->_type = done;
	reqData.clear();

	if (this->_values["BODY"].size() > this->_max_body_size)
	{
		std::cout << " >>>> Exceeded max body size: " << this->_values["BODY"].size() << "/"
				  << this->_max_body_size << std::endl;
		return true;
	}
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

void	Request::clear(void)
{
	this->_values.clear();
	this->_data.clear();
	this->_loops = 0;
	this->_type = none;
	this->_length = 0;
	this->_dataAvailible = false;
	return ;
}

void	Request::setMaxBodySize(const std::size_t mbs)
{
	this->_max_body_size = mbs;
}
