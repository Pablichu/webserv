#pragma once

#include <iostream>
#include <map>

/*enum	State//May not be necessary
{
	Unused,
	InProgress,
	Finished
};*/

struct DataState
{
	bool		state;
	std::string	data;
};

class	Request
{
	private:
		//Everything will be stored in this map
		std::map<std::string, std::string> _values;
		bool		_dataState;
		std::string	_data;
		size_t		_loops;

	public:
		Request();
		~Request();

	    void  process(void);
		const std::string	getPetit(std::string petition);

		std::map<std::string, std::string>::iterator	begin();
		std::map<std::string, std::string>::iterator	end();
		std::map<std::string, std::string> &	getHeaders(void);

		bool &			getDataSate(void);
		std::string &	getData(void);
		size_t 			updateLoop(bool loop);
};
