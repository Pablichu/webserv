#pragma once

#include <iostream>
#include <map>

class	Request
{
	private:
		//Everything will be stored in this map
		std::map<std::string, std::string> _values;

	public:
		Request();
		~Request();

    void  process(std::string reqData);
		const std::string	getPetit(std::string petition);
		std::map<std::string, std::string>::iterator	begin();
		std::map<std::string, std::string>::iterator	end();
};
