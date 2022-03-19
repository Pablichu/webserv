#include "webserv.hpp"

int		main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cout << "Please enter only one argument" << std::endl;
		return 1;
	}
	Response	res(argv[1]);

	std::cout << res.get() << std::endl;

	return 0;
}
