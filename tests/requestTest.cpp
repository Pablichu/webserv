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
  std::map<std::string, std::string>::iterator  it;
	Request                                       req;

	std::string header = getFileInString();
	std::cout << "_____ txt EXAMPLE _____" << std::endl << header << std::endl;

	req.process(header);
	std::cout << std::endl << "_____ Test with Map _____" << std::endl;

	for (it = req.begin(); it != req.end(); it++) {
		std::cout << it->first << " = " << it->second << std::endl;
	}
	return 0;
}
