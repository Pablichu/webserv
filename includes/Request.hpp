#pragma once

#include <iostream>
#include <map>
#include <sstream>
#include <stdlib.h>
#include <string.h>

enum	bodyType
{
	none,
	normal,
	chunked,
	done
};

class	Request
{
	private:
		//Everything will be stored in this map
		std::map<std::string, std::string> _values;
		std::string	_data;
		size_t		_loops;
		bodyType	_type;
		size_t		_length;
		size_t		_max_body_size;
		bool		_dataAvailible;

		size_t	_hextodec(std::string hex);
		size_t	_stoi_mine(std::string nb);

	public:
		Request(std::size_t max_body_size);
		~Request();

	    void  process(void);
		const std::string	getPetit(std::string petition);

		std::map<std::string, std::string>::iterator	begin();
		std::map<std::string, std::string>::iterator	end();
		std::map<std::string, std::string> &	getHeaders(void);

		enum bodyType &		getDataSate(void);
		std::string &		getData(void);
		bool &				dataAvailible(void);
		size_t 				updateLoop(bool loop);
		bool				processWhat();
		bool				processChunked();
		bool				processBody();
		const std::string	stoupper(std::string src);
};
