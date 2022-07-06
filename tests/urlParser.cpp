#include "UrlParser.hpp"
#include <assert.h>

void  onlyPath(void)
{
  UrlParser                           urlParser;
  std::string                         input;
  std::map<std::string, std::string>  res;

  input = "/";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/");
  assert(!res.count("FileName") && !res.count("Path_Info")
          && !res.count("Query_String"));
  res.clear();
  input = "/hello";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/hello");
  assert(!res.count("FileName") && !res.count("Path_Info")
          && !res.count("Query_String"));
  res.clear();
  input = "/hello/world";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/hello/world");
  assert(!res.count("FileName") && !res.count("Path_Info")
          && !res.count("Query_String"));
  res.clear();
  input = "/hello/how/are/you/";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/hello/how/are/you/");
  assert(!res.count("FileName") && !res.count("Path_Info")
          && !res.count("Query_String"));
  res.clear();
  input = "/hello//world/";
  assert(urlParser.parse(input, res) == true);
  /*
  **  If // is found, erase one /
  */
  assert(res["Path"] == "/hello//world/");
  assert(!res.count("FileName") && !res.count("Path_Info")
          && !res.count("Query_String"));
  res.clear();
  input = "/ hello/world ";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/ hello/world ");
  assert(!res.count("FileName") && !res.count("Path_Info")
          && !res.count("Query_String"));
  res.clear();
}

void  file(void)
{
  UrlParser                           urlParser;
  std::string                         input;
  std::map<std::string, std::string>  res;

  input = "/file.html";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/file.html");
  assert(res["FileName"] == "file.html");
  assert(!res.count("Path_Info") && !res.count("Query_String"));
  res.clear();
  input = "/I/want/this/file.html/";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/I/want/this/file.html");
  assert(res["FileName"] == "file.html");
  /*
  **  If Path_Info has data and fileName is not a CGI script,
  **  return NOT FOUND error page to user.
  */
  assert(res["Path_Info"] == "/");
  assert(!res.count("Query_String"));
  res.clear();
  input = "/file.php";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/file.php");
  assert(res["FileName"] == "file.php");
  assert(!res.count("Path_Info") && !res.count("Query_String"));
  res.clear();
}

void  query(void)
{
  UrlParser                           urlParser;
  std::string                         input;
  std::map<std::string, std::string>  res;

  input = "/?hello=hola&goodbye=adios";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/");
  assert(res["Query_String"] == "hello=hola&goodbye=adios");
  assert(!res.count("FileName") && !res.count("Path_Info"));
  res.clear();
  input = "/intro?good_morning=buenos+dias";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/intro");
  assert(res["Query_String"] == "good_morning=buenos+dias");
  assert(!res.count("FileName") && !res.count("Path_Info"));
  res.clear();
  input = "/search/?come_on=vamos";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/search/");
  assert(res["Query_String"] == "come_on=vamos");
  assert(!res.count("FileName") && !res.count("Path_Info"));
  res.clear();
  input = "/search/something.cgi?come_on=vamos";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/search/something.cgi");
  assert(res["FileName"] == "something.cgi");
  assert(res["Query_String"] == "come_on=vamos");
  assert(!res.count("Path_Info"));
  res.clear();
  /*
  **  This case should not happen, as extra ? should be escaped
  **  as %3F.
  */
  input = "/?why?=por+que?";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/");
  assert(res["Query_String"] == "why?=por+que?");
  assert(!res.count("FileName") && !res.count("Path_Info"));
  res.clear();
  input = "/?";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/");
  assert(res["Query_String"] == "");
  assert(!res.count("FileName") && !res.count("Path_Info"));
  res.clear();
}

void  pathInfo(void)
{
  UrlParser                           urlParser;
  std::string                         input;
  std::map<std::string, std::string>  res;

  input = "/file.html/";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/file.html");
  assert(res["FileName"] == "file.html");
  assert(res["Path_Info"] == "/");
  assert(!res.count("Query_String"));
  res.clear();
  input = "/file.html/hello";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/file.html");
  assert(res["FileName"] == "file.html");
  assert(res["Path_Info"] == "/hello");
  assert(!res.count("Query_String"));
  res.clear();
  input = "/file.html/hello/";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/file.html");
  assert(res["FileName"] == "file.html");
  assert(res["Path_Info"] == "/hello/");
  assert(!res.count("Query_String"));
  res.clear();
  input = "/file.html/hello/world/";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/file.html");
  assert(res["FileName"] == "file.html");
  assert(res["Path_Info"] == "/hello/world/");
  assert(!res.count("Query_String"));
  res.clear();
}

void  mix(void)
{
  UrlParser                           urlParser;
  std::string                         input;
  std::map<std::string, std::string>  res;

  input = "/hello/file.html/goodbye?hola=adios";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/hello/file.html");
  assert(res["FileName"] == "file.html");
  assert(res["Path_Info"] == "/goodbye");
  assert(res["Query_String"] == "hola=adios");
  res.clear();
  input = "/hello/file.html/goodbye/world/?hola=adios";
  assert(urlParser.parse(input, res) == true);
  assert(res["Path"] == "/hello/file.html");
  assert(res["FileName"] == "file.html");
  assert(res["Path_Info"] == "/goodbye/world/");
  assert(res["Query_String"] == "hola=adios");
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
