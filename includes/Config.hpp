#pragma once

#include "webserv.hpp"

class	Config
{
	private:
		//Config file path
		std::string					_path;

		bool								_validPath(void) const;
		bool								_validFile(void) const;

	public:
		Config(void);
		~Config(void);

		std::string const &	getPath(void) const;
		void								setPath(std::string const & path);

		bool								isValid(void) const;
};
