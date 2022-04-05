#include "Request.hpp"

Petition::Petition(std::ifstream request)
{
	std::string	line;
	int			pos;
	int			rpos;

	std::getline(request, line);

	pos = line.find(" ");
	rpos = line.rfind(" ");
	type = line.substr(0, pos - 1);
	path = line.substr(pos + 1, rpos - pos);
	protocol = line.substr(rpos + 1);
}

Petition::~Petition() {}

/*
	> GET / HTTP/1.1
	> Host: google.com
	> User-Agent: curl/7.54.0
	> Accept: */*
*/