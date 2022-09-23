#pragma once

#include <vector>
#include <stack>
#include <map>
#include <set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <climits>
#include <unistd.h>
#include <sys/stat.h>

struct LocationConfig
{
	std::string						uri;
	std::string						root;
	std::set<std::string>	methods;
	std::string						redirection;
	bool									dir_list;
	std::string						default_file;
	std::string						upload_dir;
	std::string						cgi_dir;

	LocationConfig(void);

	bool	setProperty(std::pair<std::string, std::string> & pr);
	bool	isUserDefined(std::string const & property);

private:

	//Data structure to store which properties were defined by the user
	std::set<std::string>	_userDefined;

	bool	_setMethods(std::string const & value);
	bool	_validUri(std::string const & uri);
};


struct	ServerConfig
{
	std::size_t													port; // max. port number 65535
	std::set<std::string>								server_name; // host
	std::string													error_page_dir;
	std::size_t													max_body_size;
	std::map<std::string, std::string>	cgiInterpreter;
	//Dynamic array to store different location configs
	std::vector< LocationConfig >				location;

	ServerConfig(void);

	bool	setProperty(std::pair<std::string, std::string> & pr);
	bool	isUserDefined(std::string const & property);

private:

	//Data structure to store which properties were defined by the user
	std::set<std::string>	_userDefined;

	bool	_setServerName(std::string const & value);
	bool	_processCgiPair(std::string const & value);
	bool	_setCgiInterpreter(std::string const & value);
};

/*
**	Config class which has ServerConfig/s and LocationConfig/s inside
*/

class	Config
{

private:
	//Config file path
	std::string									_path;
	//Dynamic array to store different server configs
	std::vector< ServerConfig >	_serverConfig;

	bool	_validConfigPath(void) const;

	bool	_checkMinData(void);

	bool	_isServerProperty(std::string const & prop);
	bool	_isLocationProperty(std::string const & prop);

	//Data extraction methods
	bool	_extractProperty(std::string const & token, std::size_t & pos,
													std::stack<std::pair<char, std::string> > & state,
													std::pair<std::string, std::string> & prop);
	void	_extractMultiStruct(char bracket, std::size_t & pos,
												std::stack<std::pair<char, std::string> > & state);
	void	_extractStruct(char brace, std::size_t & pos,
												std::stack<std::pair<char, std::string> > & state);
	bool	_extractData(std::vector<std::string> const & tokens);

	//Validation methods
	bool	_processBraces(char brace, std::size_t & pos,
												std::stack< std::pair<char, std::string> > & state);
	bool	_processBrackets(char bracket, std::size_t & pos,
												std::stack< std::pair<char, std::string> > & state);
	bool  _processComma(std::size_t & pos,
												std::stack< std::pair<char, std::string> > & state);
	bool  _processColon(std::size_t & pos,
												std::stack< std::pair<char, std::string> > & state);
	bool  _processString(std::string const & token, std::size_t & pos,
												std::stack< std::pair<char, std::string> > & state);
	bool  _processDigits(std::string const & token, std::size_t & pos,
												std::stack< std::pair<char, std::string> > & state);
	bool	_validateSyntax(std::vector<std::string> const & tokens);
	
	void	_tokenizeFile(std::vector<std::string> & tokens);

public:

	Config(void);
	~Config(void);

	static bool					validDirectoryPath(std::string & path);

	std::string const &	getPath(void) const;
	void								setPath(std::string const & path);

	bool								parseFile(void);

	//NEED TO DISCUSS HOW TO ACCESS THE CONFIG VALUES
	std::vector< ServerConfig > const &	getConfig(void) const;

};
