#include "Response.hpp"

Response::Response(std::string file_route)
{
	protocol = "HTTP/1.1";
	status = 200;
	if (file_route.find("404") != std::string::npos)
		status = 404;
	status_msg = "OK";
	if (status != 200)
		status_msg = "Not Found";

	std::string		doctype
	(
		file_route.substr
		(
			file_route.find_last_of('.') + 1,
			file_route.size() - file_route.find_last_of('.') + 1
		)
	);
	std::ifstream	file(file_route.c_str());

	cnt_type = "text/" + doctype + "; charset=utf-8"; //TODO: cambiar en futuro

	body = std::string((std::istreambuf_iterator<char>(file)),
						std::istreambuf_iterator<char>());

	cnt_length = body.size();
}

Response::~Response() {}

std::string		Response::get()
{
	std::stringstream	rsp;

	rsp << protocol << " " << status << " " << status_msg << std::endl;
	rsp << "Content-length: " << cnt_length << std::endl;
	rsp << "Content-type: " << cnt_type << std::endl;
	rsp << std::endl;
	rsp << body;

	return rsp.str();
}
