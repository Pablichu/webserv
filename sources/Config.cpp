#include "Config.hpp"
#include <cstdlib>

Config::Config(void) : _path("default/path")
{
  return ;
}

Config::~Config(void)
{
  return ;
}

std::string const & Config::getPath(void) const
{
  return (this->_path);
}

void  Config::setPath(std::string const & path)
{
  this->_path = path;
  return ;
}

bool  Config::isValid(void)
{
  if (this->_path == ""
      || !this->_validPath()
      || !this->_validFile())
    return (false);
  return (true);
}

//PRIVATE METHODS

bool  Config::_validPath(void) const
{
  std::size_t   file_type_pos;
  std::ifstream file;

  //Check file type
  file_type_pos = this->_path.find(".json");
  if ( this->_path.length() <= 5
      || file_type_pos == std::string::npos
      || file_type_pos + 5 != this->_path.length() )
    return (false);
  //Check file exists
  file.open(this->_path.c_str()); //C++98 ifstream doesn't accept std::string
  if (!file.is_open())
    return (false);
  file.close();
  return (true);
}

bool  Config::_checkMinConfig(void) const
{
  //PENDING ...
  //NEED TO DISCUSS WHAT OR IF THERE ARE MINIMUM CONFIG PROPERTIES
  return (true);
}

bool  Config::_isServerProperty(std::string const & prop)
{
  if (prop != "port" && prop != "host" && prop != "server_name"
        && prop != "not_found_page" && prop != "max_body_size"
        && prop != "location")
    return (false);
  return (true);
}

bool  Config::_isLocationProperty(std::string const & prop)
{
  if (prop != "uri" && prop != "root" && prop != "methods"
        && prop != "redirection" && prop != "dir_list"
        && prop != "default_file" && prop != "upload_dir")
    return (false);
  return (true);
}

bool  Config::_processBraces(char brace, std::size_t & pos,
  std::stack< std::pair<char, std::string> > & state)
{
  if (brace == '{')
  {
    if (state.empty()) //Main brackets
      state.push(std::pair<char, std::string>('{', ""));
    else if (state.top().first == ',') //Next of multi_server or multi_location
    {
      state.pop();
      state.push(std::pair<char, std::string>('{', state.top().second));
      if (state.top().second == "server")
        this->_serverConfig.push_back(ServerConfig());
      else if (state.top().second == "location")
        this->_serverConfig.back().location.push_back(LocationConfig());
      else
        return (false);
    }
    else if (state.top().first == '[') //First of multi_server or multi_location
    {
      state.push(std::pair<char, std::string>('{', state.top().second));
      if (state.top().second == "server")
        this->_serverConfig.push_back(ServerConfig());
      else
        this->_serverConfig.back().location.push_back(LocationConfig());
    }
    else if (state.top().first == ':'
              && (state.top().second == "server"
              || state.top().second == "location"))
    {
      state.top().first = '{';
      if (state.top().second == "server")
        this->_serverConfig.push_back(ServerConfig());
      else if (!this->_serverConfig.empty()
                && this->_serverConfig.back().location.empty())
        this->_serverConfig.back().location.push_back(LocationConfig());
      else
        return (false);
    }
    else
      return (false);
  }
  else // if brace == '}'
  {
    if (state.top().first == '}')
    {
      state.pop();
      if (state.top().first != '{')
        return (false);
    }
    else if (state.top().first == ']')
    {
      state.pop();
      if (state.top().first != '[')
        return (false);
    }
    state.pop();
    if (state.top().first != '{')
      return (false);
    state.push(std::pair<char, std::string>('}', state.top().second));
  }
  pos += 1;
  return (true);
}

bool  Config::_processBrackets(char bracket, std::size_t & pos,
  std::stack< std::pair<char, std::string> > & state)
{
  if (bracket == '[')
  {
    if (state.top().first == ':'
        && (state.top().second == "server" || state.top().second == "location"))
      state.top().first = '[';
    else
      return (false);
  }
  else
  {
    if (state.top().first != '}')
      return (false);
    state.pop();
    state.pop();
    if (state.top().first != '[')
      return (false);
    state.push(std::pair<char, std::string>(']', state.top().second));
  }
  pos += 1;
  return (true);
}

bool  Config::_processComma(std::size_t & pos,
  std::stack< std::pair<char, std::string> > & state)
{
  if (state.top().first == '=')
    state.top().first = ',';
  else if (state.top().first == '}' || state.top().first == ']')
  {
    state.pop();
    state.top().first = ',';
  }
  else
    return (false);
  pos += 1;
  return (true);
}

bool  Config::_processColon(std::size_t & pos,
  std::stack< std::pair<char, std::string> > & state)
{
  if (!state.top().first && state.top().second != "")
    state.top().first = ':';
  else
    return (false);
  pos += 1;
  return (true);
}

bool  Config::_processString(std::string const & token, std::size_t & pos,
  std::stack< std::pair<char, std::string> > & state)
{
  std::string                         val;
  std::size_t                         end_quote;
  std::pair<std::string, std::string> prop;

  end_quote = token.find("\"", pos + 1);
  if (end_quote == std::string::npos)
    return (false);
  val = token.substr(pos + 1, end_quote - (pos + 1));
  if (state.top().first == '{')
  {
    if (val == "server" && state.top().second == "")
      state.push(std::pair<char, std::string>(0, "server"));
    else if ( (state.top().second == "server" && this->_isServerProperty(val))
            || (state.top().second == "location"
                && this->_isLocationProperty(val)) )
      state.push(std::pair<char, std::string>(0, val));
    else
      return (false);
  }
  else if (state.top().first == ',')
  {
    if ( (this->_isServerProperty(state.top().second)
            == this->_isServerProperty(val))
          || (this->_isLocationProperty(state.top().second)
            == this->_isLocationProperty(val)))
    {
      state.pop();
      state.push(std::pair<char, std::string>(0, val));
    }
    else
      return (false);
  }
  else if (state.top().first == ':'
            && (this->_isServerProperty(state.top().second)
                || this->_isLocationProperty(state.top().second)))
  {
    prop.first = state.top().second;
    prop.second = val;
    //Checks to which structure the property belongs and tries to set it.
    if ( (this->_isServerProperty(prop.first)
          && !this->_serverConfig.back().setProperty(prop))
        || (this->_isLocationProperty(prop.first)
          && !this->_serverConfig.back().location.back().setProperty(prop)))
      return (false);
    state.top().first = '=';
  }
  else
    return (false);
  pos += val.length() + 2;
  return (true);
}

bool  Config::_processDigits(std::string const & token, std::size_t & pos,
  std::stack< std::pair<char, std::string> > & state)
{
  std::string                         val;
  std::size_t                         end;
  std::pair<std::string, std::string> prop;

  if (state.top().first != ':')
    return (false);
  end = pos;
  while (token[end] && std::isdigit(token[end]))
    ++end;
  val = token.substr(pos, end - pos);
  if (val.length() > 5)
    return (false);
  prop.first = state.top().second;
  prop.second = val;
  //Checks to which structure the property belongs and tries to set it.
  if ( (this->_isServerProperty(prop.first)
          && !this->_serverConfig.back().setProperty(prop))
        || (this->_isLocationProperty(prop.first)
          && !this->_serverConfig.back().location.back().setProperty(prop)))
    return (false);
  state.top().first = '=';
  pos += val.length();
  return (true);
}

/*
**  Loops through each token and each of its characacters checking
**  for syntax errors and validating and extracting config data.
**
**  state is a stack that stores the previous interpreted json symbol
**  and property as a pair to provide context for syntax validation.
*/

bool  Config::_getConfigData(std::vector<std::string> const & tokens)
{
  std::stack< std::pair<char, std::string> >  state;
  std::vector<std::string>::const_iterator    it;
  std::size_t                                 pos;
  bool                                        valid;

  if (tokens.empty() || tokens[0][0] != '{')
    return (false);
  for (it = tokens.begin(); it != tokens.end(); ++it)
  {
    pos = 0;
    while ((*it)[pos])
    {
      //pos gets updated inside process methods
      if ((*it)[pos] == '{' || (*it)[pos] == '}')
        valid = this->_processBraces((*it)[pos], pos, state);
      else if ((*it)[pos] == '[' || (*it)[pos] == ']')
        valid = this->_processBrackets((*it)[pos], pos, state);
      else if ((*it)[pos] == '"')
        valid = this->_processString(*it, pos, state);
      else if (isdigit((*it)[pos]))
        valid = this->_processDigits(*it, pos, state);
      else if ((*it)[pos] == ',')
        valid = this->_processComma(pos, state);
      else if ((*it)[pos] == ':')
        valid = this->_processColon(pos, state);
      else
        valid = false;
      if (!valid)
        return (false);
    }
  }
  if (state.top().first != '}' || state.top().second != "")
    return (false);
  return (true);
}

void  Config::_tokenizeLine(std::string & line,
        std::vector<std::string> & tokens)
{
  std::stringstream line_stream;
  std::string       token;

  //Trim left and right whitespace
  line.erase(0, line.find_first_not_of(" \n\r\t"));                                                                                               
  line.erase(line.find_last_not_of(" \n\r\t") + 1);
  //Pass line to stringstream
  line_stream << line;
  //Get substrings delimited by spaces from stringstream
  while (std::getline(line_stream, token, ' '))
  {
    //Prevents adding empty string when multiple spaces are found between tokens
    if (token != "")
      tokens.push_back(token);
  }
  return ;
}

bool  Config::_validFile(void)
{
  std::ifstream             file(this->_path.c_str());
  std::string               line;
  std::vector<std::string>  tokens;
  bool                      valid;

  //Tokenize all json file lines
  while (std::getline(file, line))
    this->_tokenizeLine(line, tokens);
  // Adds data to config and returns false if bad syntax is encountered
  valid = this->_getConfigData(tokens);
  //Check if config file has minimum data for the server to work
  if (valid && !this->_checkMinConfig())
    valid = false;
  file.close();
  return (valid);
}

//ServerConfig STRUCTURE METHOD DEFINITIONS

ServerConfig::ServerConfig(void) : port(0), host(""), not_found_page(""),
  max_body_size(0)
{
  return ;
}

/*
**  Adds a property to the struct if data is valid and was not inserted before.
*/

bool  ServerConfig::setProperty(std::pair<std::string, std::string> & pr)
{
  std::string     prop;
  std::string     val;

  prop = pr.first;
  val = pr.second;
  if (prop == "port" && (this->_userDefined.insert(prop)).second)
  {
    this->port = atoi(val.c_str()); //Implicit conversion
    if (this->port > 65535)
      return (false);
  }
  else if (prop == "host" && (this->_userDefined.insert(prop)).second)
    this->host = val; // validate name
  else if (prop == "server_name" && (this->_userDefined.insert(prop)).second)
  {
    if (!this->_setServerName(val))
      return (false);
  }
  else if (prop == "not_found_page" && (this->_userDefined.insert(prop)).second)
    this->not_found_page = val; //validate path
  else if (prop == "max_body_size" && (this->_userDefined.insert(prop)).second)
  {
    this->max_body_size = atoi(val.c_str()); //Implicit conversion
    if (this->max_body_size > 8192)
      return (false);
  }
  else
    return (false);
  return (true);
}

/*
**  Determines if property was provided by the user. It checks if
**  it was inserted in the _userDefined set data structure when
**  processing the config file data.
*/

bool  ServerConfig::isUserDefined(std::string const & property)
{
  if (this->_userDefined.find(property) == this->_userDefined.end())
    return (false);
  return (true);
}

/*
**  Insert one or more names, which are delimited by commas if more than one,
**  to the server_name set data structure.
*/

bool  ServerConfig::_setServerName(std::string const & value)
{
  std::stringstream ss;
  std::string       name;
  bool              no_repeat;

  ss << value;
  while (std::getline(ss, name, ',')) //If value is empty doesn't enter loop
  {
    if (name == "") //Double comma or starting comma found
      return (false);
    no_repeat = (this->server_name.insert(name)).second;
    if (!no_repeat) // NAME WAS ALREADY INSERTED IN set
      return (false);
  }
  if (value != "" && value[value.length() - 1] == ',') //Found comma at the end
    return (false);
  return (true);
}

//LocationConfig STRUCTURE METHOD DEFINITIONS

LocationConfig::LocationConfig(void) : uri(""), root(""), redirection(""),
  dir_list(false), default_file(""), upload_dir("")
{
  return ;
}

/*
**  Adds a property to the struct if data is valid and was not inserted before.
*/

bool  LocationConfig::setProperty(std::pair<std::string, std::string> & pr)
{
  std::string prop;
  std::string val;

  prop = pr.first;
  val = pr.second;
  if (prop == "uri" && (this->_userDefined.insert(prop)).second)
    this->uri = val; // validate uri
  else if (prop == "root" && (this->_userDefined.insert(prop)).second)
    this->root = val; // validate path
  else if (prop == "methods" && (this->_userDefined.insert(prop)).second)
  {
    if (!this->_setMethods(val))
      return (false);
  }
  else if (prop == "redirection" && (this->_userDefined.insert(prop)).second)
    this->redirection = val; // validate uri
  else if (prop == "dir_list" && (this->_userDefined.insert(prop)).second)
  {
    if (atoi(val.c_str()) == 1)
      this->dir_list = true;
    else if (atoi(val.c_str()) == 0)
      this->dir_list = false;
    else
      return (false);
  }
  else if ("default_file" && (this->_userDefined.insert(prop)).second)
    this->default_file = val; // validate path
  else if ("upload_dir" && (this->_userDefined.insert(prop)).second)
    this->upload_dir = val; // validate path
  else
    return (false);
  return (true);
}

/*
**  Determines if property was provided by the user. It checks if
**  it was inserted in the _userDefined set data structure when
**  processing the config file data.
*/

bool  LocationConfig::isUserDefined(std::string const & property)
{
  if (this->_userDefined.find(property) == this->_userDefined.end())
    return (false);
  return (true);
}

/*
**  Insert one or more methods, which are delimited by commas if more than one,
**  to the methods set data structure.
*/

bool  LocationConfig::_setMethods(std::string const & value)
{
  std::stringstream ss;
  std::string       method;
  bool              no_repeat;

  ss << value;
  while (std::getline(ss, method, ','))
  {
    if (method == "" //Double comma or starting comma found
          || (method != "GET" && method != "POST" && method != "DELETE"))
      return (false);
    no_repeat = (this->methods.insert(method)).second;
    if (!no_repeat) // METHOD WAS ALREADY INSERTED IN set
      return (false);
  }
  if (value[value.length() - 1] == ',') //Found comma at the end
    return (false);
  return (true);
}