#include "Request.hpp"

Request::Request() : _loops(0), _type(none), _length(0) {}

Request::~Request() {}

/**
*  To avoid generating a copy of reqData, consider passing
*  std::string const & reqData, but to do this, reqData.erase() cannot be used.
*/
void  Request::process()
{
	std::string		headerName;
	size_t				pos;
	size_t				rpos;
	std::string &	reqData = this->_data;

	//First line
	pos = reqData.find(" ");
	this->_values["METHOD"] = reqData.substr(0, pos);
	pos = reqData.find_first_not_of(' ', pos);
	rpos = reqData.find(' ', pos);
	this->_values["PATH"] = reqData.substr(pos, rpos - pos);
	pos = reqData.find_first_not_of(' ', rpos);
	rpos = reqData.find_first_of("\r\n", pos);
	this->_values["PROTOCOL"] = reqData.substr(pos, rpos - pos);
	//Get rest of reqData
	while(true)
	{
		reqData.erase(0, rpos + (reqData[rpos] == '\r' ? 2 : 1));
		//This takes the body in case there is
		if (reqData[0] == '\r' || reqData[0] == '\n')
		{// Line terminations in HTTP messages are \r\n
			reqData.erase(0, reqData[0] == '\r' ? 2 : 1);
			break ;
		}
		pos = reqData.find(":");
		headerName = reqData.substr(0, pos);
		std::transform(headerName.begin(), headerName.end(), headerName.begin(),
										toupper);
		pos = reqData.find_first_not_of(' ', pos + 2);
		rpos = reqData.find_first_of("\r\n");
		if (rpos == std::string::npos)
			break;
		this->_values[headerName] = reqData.substr(pos, rpos - pos);
		utils::removeWhiteSpace(this->_values[headerName]);
	}
	if (this->getPetit("TRANSFER-ENCODING") == "chunked")//Check, maybe some parts are not neccessary
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

void	Request::processChunked()
{
	std::string &				body = this->_values["BODY"];
	std::string &				data = this->_data;
	std::size_t					pos;
	std::size_t					chunkSize;
	static std::size_t	chunkDataLeft = 0;

	while (!data.empty())
	{
		if (data.find("0\r\n\r\n") == 0) //found at position 0
		{
			this->_type = done;
			data.clear();
			std::cout << "Mensaje de felicidad extrema" << std::endl;
			return ;
		}
		if (!chunkDataLeft)
		{
			pos = data.find("\r\n");
			if (pos == std::string::npos)
				return ;
			chunkDataLeft = _hextodec(data.substr(0, pos));
			data.erase(0, pos + 2);
		}
		/*
		**	If data length is smaller than the remaining size of the current chunk,
		**	it means that all data's content is part of the current chunk.
		**
		**	If data length is bigger or equal to the remaining size of the current
		**	chunk, it means that data has more content in addition to the current
		**	chunk's content.
		*/
		chunkSize = data.length() < chunkDataLeft ? data.length() : chunkDataLeft;
		body.append(data.substr(0, chunkSize));
		data.erase(0, chunkSize);
		chunkDataLeft -= chunkSize;
		if(data[0] == '\r')
			data.erase(0, 2);
	}
}

void	Request::processBody()
{
	std::string &				reqData = this->_data;
	static std::size_t	bodyDataLeft = 0;

	if (!reqData.length())
		return ;
	if (!bodyDataLeft) //First call to read body
		bodyDataLeft = this->_length; //How much data must be read in total
	this->_values["BODY"].append(reqData.substr());
	bodyDataLeft -= reqData.length();
	/*
	**	Always empty reqData after appending to Body to prevent
	**	unnecessary duplicates
	*/
	reqData.clear();
	if (!bodyDataLeft)
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
