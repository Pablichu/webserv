#include "Request.hpp"
#include <vector>
#include <assert.h>

void  emptyBody(void)
{
  Request *                                 req;
  std::vector<std::string>                  payload;
  std::vector<std::string>::const_iterator  it;
  std::size_t                               i;

  payload.push_back("GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\n\r\n");
  payload.push_back("GET    /index.html    HTTP/1.1\r\nHost:   localhost:8080\r\n\r\n");
  payload.push_back("GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\n\r\n\r\n");
  for (it = payload.begin(), i = 0; it != payload.end(); ++it, ++i)
  {
    std::cout << "emptyBody iteration number: " << i << std::endl;
    req = new Request;
    req->getData().append(*it);
    req->process();
    assert(req->getPetit("METHOD") == "GET"
            && req->getPetit("PATH") == "/index.html"
            && req->getPetit("PROTOCOL") == "HTTP/1.1");
    assert(req->getPetit("HOST") == "localhost:8080");
    assert(req->getHeaders().size() == 4);
    delete req;
  }
  return ;
}

void  regularBody(void)
{
  Request *                 req;
  std::vector<std::string>  payload;
  std::vector<std::string>  body;

  payload.push_back(
    "GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 12\r\n\r\nHello World!");
  payload.push_back(
    "GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 26\r\n\r\nHello World!\nHow are you?\n");
  payload.push_back(
    "GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 27\r\n\r\nHello World!\r\nHow are you?\n");
  payload.push_back(
    "GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 29\r\n\r\nHello World!\r\n\r\nHow are you?\n");
  body.push_back("Hello World!");
  body.push_back("Hello World!\nHow are you?\n");
  body.push_back("Hello World!\r\nHow are you?\n");
  body.push_back("Hello World!\r\n\r\nHow are you?\n");
  for (std::size_t i = 0; i < payload.size(); ++i)
  {
    std::cout << "regularBody iteration number: " << i << std::endl;
    req = new Request;
    req->getData().append(payload[i]);
    req->process();
    assert(req->getPetit("METHOD") == "GET"
            && req->getPetit("PATH") == "/index.html"
            && req->getPetit("PROTOCOL") == "HTTP/1.1");
    assert(req->getPetit("HOST") == "localhost:8080");
    assert(req->getPetit("CONTENT-LENGTH") != "");
    assert(req->getPetit("BODY") == body[i]);
    assert(req->getHeaders().size() == 6);
    delete req;
  }
  return ;
}

int main(void)
{
  std::cout << "\n--- REQUEST TESTS ---\n";
  emptyBody();
  std::cout << "\nEMPTY BODY: OK\n";
  regularBody();
  std::cout << "\nREGULAR BODY: OK\n";
  std::cout << std::endl;
  return (0);
}
