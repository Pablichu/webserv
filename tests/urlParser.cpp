#include "UrlParser.hpp"
#include <assert.h>

void  onlyPath(void)
{
  UrlParser                           urlParser;
  std::string                         input;
  std::map<std::string, std::string>  res;

  input = "/";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/");
  assert(!res.count("FILENAME") && !res.count("PATH_INFO")
          && !res.count("QUERY_STRING"));
  res.clear();
  input = "/hello";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/hello");
  assert(!res.count("FILENAME") && !res.count("PATH_INFO")
          && !res.count("QUERY_STRING"));
  res.clear();
  input = "/hello/world";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/hello/world");
  assert(!res.count("FILENAME") && !res.count("PATH_INFO")
          && !res.count("QUERY_STRING"));
  res.clear();
  input = "/hello/how/are/you/";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/hello/how/are/you/");
  assert(!res.count("FILENAME") && !res.count("PATH_INFO")
          && !res.count("QUERY_STRING"));
  res.clear();
  input = "/hello//world/";
  assert(urlParser.parse(input, res) == true);
  /*
  **  If // is found, erase one /
  */
  assert(res["PATH"] == "/hello//world/");
  assert(!res.count("FILENAME") && !res.count("PATH_INFO")
          && !res.count("QUERY_STRING"));
  res.clear();
  input = "/ hello/world ";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/ hello/world ");
  assert(!res.count("FILENAME") && !res.count("PATH_INFO")
          && !res.count("QUERY_STRING"));
  res.clear();
}

void  file(void)
{
  UrlParser                           urlParser;
  std::string                         input;
  std::map<std::string, std::string>  res;

  input = "/file.html";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/file.html");
  assert(res["FILENAME"] == "file.html");
  assert(res["FILETYPE"] == ".html");
  assert(!res.count("PATH_INFO") && !res.count("QUERY_STRING"));
  res.clear();
  input = "/I/want/this/file.html/";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/I/want/this/file.html");
  assert(res["FILENAME"] == "file.html");
  assert(res["FILETYPE"] == ".html");
  /*
  **  If Path_Info has data and fileName is not a CGI script,
  **  return NOT FOUND error page to user.
  */
  assert(res["PATH_INFO"] == "/");
  assert(!res.count("QUERY_STRING"));
  res.clear();
  input = "/file.php";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/file.php");
  assert(res["FILENAME"] == "file.php");
  assert(res["FILETYPE"] == ".php");
  assert(!res.count("PATH_INFO") && !res.count("QUERY_STRING"));
  res.clear();
}

void  query(void)
{
  UrlParser                           urlParser;
  std::string                         input;
  std::map<std::string, std::string>  res;

  input = "/?hello=hola&goodbye=adios";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/");
  assert(res["QUERY_STRING"] == "hello=hola&goodbye=adios");
  assert(!res.count("FILENAME") && !res.count("PATH_INFO"));
  res.clear();
  input = "/intro?good_morning=buenos+dias";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/intro");
  assert(res["QUERY_STRING"] == "good_morning=buenos+dias");
  assert(!res.count("FILENAME") && !res.count("PATH_INFO"));
  res.clear();
  input = "/search/?come_on=vamos";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/search/");
  assert(res["QUERY_STRING"] == "come_on=vamos");
  assert(!res.count("FILENAME") && !res.count("PATH_INFO"));
  res.clear();
  input = "/search/something.cgi?come_on=vamos";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/search/something.cgi");
  assert(res["FILENAME"] == "something.cgi");
  assert(res["FILETYPE"] == ".cgi");
  assert(res["QUERY_STRING"] == "come_on=vamos");
  assert(!res.count("PATH_INFO"));
  res.clear();
  /*
  **  This case should not happen, as extra ? should be escaped
  **  as %3F.
  */
  input = "/?why?=por+que?";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/");
  assert(res["QUERY_STRING"] == "why?=por+que?");
  assert(!res.count("FILENAME") && !res.count("PATH_INFO"));
  res.clear();
  input = "/?";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/");
  assert(res["QUERY_STRING"] == "");
  assert(!res.count("FILENAME") && !res.count("PATH_INFO"));
  res.clear();
}

void  pathInfo(void)
{
  UrlParser                           urlParser;
  std::string                         input;
  std::map<std::string, std::string>  res;

  input = "/file.html/";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/file.html");
  assert(res["FILENAME"] == "file.html");
  assert(res["FILETYPE"] == ".html");
  assert(res["PATH_INFO"] == "/");
  assert(!res.count("QUERY_STRING"));
  res.clear();
  input = "/file.html/hello";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/file.html");
  assert(res["FILENAME"] == "file.html");
  assert(res["FILETYPE"] == ".html");
  assert(res["PATH_INFO"] == "/hello");
  assert(!res.count("QUERY_STRING"));
  res.clear();
  input = "/file.html/hello/";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/file.html");
  assert(res["FILENAME"] == "file.html");
  assert(res["FILETYPE"] == ".html");
  assert(res["PATH_INFO"] == "/hello/");
  assert(!res.count("QUERY_STRING"));
  res.clear();
  input = "/file.html/hello/world/";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/file.html");
  assert(res["FILENAME"] == "file.html");
  assert(res["FILETYPE"] == ".html");
  assert(res["PATH_INFO"] == "/hello/world/");
  assert(!res.count("QUERY_STRING"));
  res.clear();
}

void  mix(void)
{
  UrlParser                           urlParser;
  std::string                         input;
  std::map<std::string, std::string>  res;

  input = "/hello/file.html/goodbye?hola=adios";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/hello/file.html");
  assert(res["FILENAME"] == "file.html");
  assert(res["FILETYPE"] == ".html");
  assert(res["PATH_INFO"] == "/goodbye");
  assert(res["QUERY_STRING"] == "hola=adios");
  res.clear();
  input = "/hello/file.html/goodbye/world/?hola=adios";
  assert(urlParser.parse(input, res) == true);
  assert(res["PATH"] == "/hello/file.html");
  assert(res["FILENAME"] == "file.html");
  assert(res["FILETYPE"] == ".html");
  assert(res["PATH_INFO"] == "/goodbye/world/");
  assert(res["QUERY_STRING"] == "hola=adios");
  res.clear();
}

int main(void)
{
  std::cout << "\n--- URL PARSER TESTS ---\n";
  onlyPath();
  std::cout << "\nONLY PATH: OK\n";
  file();
  std::cout << "\nFILE PATH: OK\n";
  query();
  std::cout << "\nQUERY: OK\n";
  pathInfo();
  std::cout << "\nPATH INFO: OK\n";
  mix();
  std::cout << "\nMIX: OK\n";
  std::cout << std::endl;
  return (0);
}
