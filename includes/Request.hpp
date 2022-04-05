#pragma once

#include "webserv.hpp"

class	Petition
{
	private:
		//Start line
		std::string	type;
		std::string	path;
		std::string	protocol;

		//Header, check what we need
		
	public:
		Petition(std::ifstream request);
		~Petition();
};
