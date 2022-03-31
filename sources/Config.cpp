#include "Config.hpp"

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
    }
    else if (state.top().first == '[') //First of multi_server or multi_location
      state.push(std::pair<char, std::string>('{', state.top().second));
    else if (!state.top().first
              && (state.top().second == "server"
              || state.top().second == "location"))
      state.top().first = '{';
    else
      return (false);
  }
  else
  {
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
    if (state.empty())
      return (false);
    if (!state.top().first
        && (state.top().second == "server" || state.top().second == "location"))
      state.top().first = '[';
    else
      return (false);
  }
  else
  {
    if (state.top().first != '[')
      return (false);
    state.push(std::pair<char, std::string>('[', state.top().second));
  }
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
    state.pop();
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
  return (true);
}

bool  Config::_processString(std::string const & token, std::size_t & pos,
  std::stack< std::pair<char, std::string> > & state)
{
  std::string val;
  std::size_t end_quote;

  end_quote = token.find("\"", pos + 1);
  if (end_quote == std::string::npos)
    return (false);
  val = token.substr(pos + 1, end_quote - pos + 1);
  //Check block: server or location
  //Check valid val for block
  return (true);
}

bool  Config::_getConfigData(std::vector<std::string> const & tokens)
{
  std::stack< std::pair<char, std::string> >  state;
  std::vector<std::string>::const_iterator    it;
  std::size_t                                 pos;
  bool                                        valid;

  if (tokens.empty())
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
      {}
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
  return (true);
}

void  Config::_tokenizeLine(std::string & line,
        std::vector<std::string> & tokens)
{
  std::stringstream line_stream;
  std::string       token;

  //Trim left and right whitespace
  line.erase(0, line.find_first_not_of(" \n\r\t"));                                                                                               
  line.erase(line.find_last_not_of(" \n\r\t")+1);
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
  //Returns false if bad syntax is encountered
  valid = this->_getConfigData(tokens);
  //Check if config file has minimum data for the server to work
  if (valid && !this->_checkMinConfig())
    valid = false;
  file.close();
  return (valid);
}

//STRUCTURE DEFAULT CONSTRUCTORS

ServerConfig::ServerConfig(void) : port(0), host(""), server_name(""),
  not_found_page(""), max_body_size(0)
{
  return ;
}

LocationConfig::LocationConfig(void) : root(""), methods(""),
  redirection(""), dir_list(false), default_file(""), upload_dir("")
{
  return ;
}