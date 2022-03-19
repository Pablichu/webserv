#include "webserv.hpp"

int		main()
{
	Response	res("main.cpp");

	std::cout << res.get() << std::endl;

	return 0;
}
