#include <assert.h>

#include "utils.hpp"

void  toString(void)
{
  int         i;
  long        l;
  std::size_t s;
  float       f;
  std::string str;

  i = 404;
  str = utils::toString<int>(i);
  assert(str == "404");
  i = -404;
  str = utils::toString<int>(i);
  assert(str == "-404");
  l = 12346789;
  str = utils::toString<long>(l);
  assert(str == "12346789");
  s = 12346789;
  str = utils::toString<std::size_t>(s);
  assert(str == "12346789");
  f = 404.44;
  str = utils::toString<float>(f);
  assert(str == "404.44");
  return ;
}

void  getFileExtension(void)
{
  std::string input;

  input = "/hello.cgi";
  assert(utils::getFileExtension(input) == ".cgi");
  input = "/hello/hello.cgi";
  assert(utils::getFileExtension(input) == ".cgi");
  input = "/hello/bye.html/hello.cgi";
  assert(utils::getFileExtension(input) == ".cgi");
  return ;
}

void  extractHost(void)
{
  std::string input;

  input = "localhost:8080";
  assert(utils::extractHost(input) == "localhost");
  input = "www.hello.com:8080";
  assert(utils::extractHost(input) == "www.hello.com");
  return ;
}

void  extractPort(void)
{
  std::string input;

  input = "localhost:8080";
  assert(utils::extractPort(input) == "8080");
  input = "www.hello.com:8080";
  assert(utils::extractPort(input) == "8080");
  return ;
}

int main(void)
{
  std::cout << "\n--- UTILS TESTS ---\n";
  toString();
  std::cout << "\ntoString: OK\n";
  getFileExtension();
  std::cout << "\ngetFileExtension: OK\n";
  extractHost();
  std::cout << "\nextractHost: OK\n";
  extractPort();
  std::cout << "\nextractPort: OK\n";
  std::cout << std::endl; 
  return (0);
}
