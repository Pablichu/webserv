#pragma once

#include "webserv.hpp"
#include <map>

class	Request
{
	private:
		//Everything will be stored in this map
		std::map<std::string, std::string> _values;

	public:
		Request(std::string header);
		~Request();

		const std::string	GetPetit(std::string petition);
		std::map<std::string, std::string>	GetWholePetit();
};
