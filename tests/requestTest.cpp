#include "Request.hpp"

std::string getFileInString()
{
	std::string s;
	std::string sTotal;

	std::ifstream in;
	in.open("tests/request.txt");

	while(!in.eof()) {
		getline(in, s);
		if (in.eof())
			sTotal += s;
		else
			sTotal += s + "\n";
	}

	in.close();	
	return sTotal;
}

int	main()
{
	std::string header = getFileInString();
	std::cout << "_____ txt EXAMPLE _____" << std::endl << header << std::endl;

	Request test(header);
	std::cout << std::endl << "_____ Test with Map _____" << std::endl;

	std::map<std::string, std::string> m = test.GetWholePetit();
	for (std::map<std::string, std::string>::const_iterator it = m.begin(); it != m.end(); it++) {
		std::cout << it->first << " = " << it->second << std::endl;
	}
	return 0;
}
