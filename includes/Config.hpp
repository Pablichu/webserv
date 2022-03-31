#pragma once

#include <map>
#include <vector>
#include <stack>

#include "webserv.hpp"

struct LocationConfig
{
	std::string	root;
	std::string	methods;
	std::string	redirection;
	bool				dir_list;
	std::string	default_file;
	std::string	upload_dir;

	LocationConfig(void);
};


struct	ServerConfig
{
	unsigned int													port; // max. port number 65535
	std::string														host;
	std::string														server_name;
	std::string														not_found_page;
	unsigned int													max_body_size;
	//map Key = location url, map Value = LocationConfig struct
	std::map<std::string, LocationConfig>	location;

	ServerConfig(void);
};

class	Config
{
	private:
		//Config file path
		std::string														_path;
		//Map data structure to store different server configs
		std::map< std::string, ServerConfig >	_serverConfig;

		bool								_validPath(void) const;
		void								_tokenizeLine(std::string & line,
													std::vector<std::string> & tokens);
		bool								_process_braces(char brace, std::size_t & pos,
													std::stack<std::pair<char, std::string>> & state);
		bool								_process_brackets(char bracket, std::size_t & pos,
													std::stack<std::pair<char, std::string>> & state);
		bool								_getConfigData(std::vector<std::string> const & tokens);
		bool								_checkMinConfig(void) const;
		bool								_validFile(void);

	public:
		Config(void);
		~Config(void);

		std::string const &	getPath(void) const;
		void								setPath(std::string const & path);

		bool								isValid(void);
};
