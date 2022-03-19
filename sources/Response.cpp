#include "Response.hpp"

Response::Response(std::string file_route)
{
	std::string		doctype
	(
		file_route.substr
		(
			file_route.find_last_of('.') + 1,
			file_route.size() - file_route.find_last_of('.') + 1
		)
	);
	std::ifstream	file(file_route.c_str());

	cnt_type = std::string("text/") + doctype + "; charset=utf-8"; //TODO: cambiar en futuro
	cnt_length = file.size();
}
