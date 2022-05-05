#include "Request.hpp"

std::string getFileInString()
{
	std::string s;
	std::string sTotal;

	std::ifstream in;
	in.open("tests/request.txt");

	while(!in.eof()) {
		getline(in, s);
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
	std::cout << "_____ My Request  _____" << std::endl
			  << "Method > |" << test.GetMethod() << "|" << std::endl
			  << "Path > |" << test.GetPath() << "|" << std::endl
			  << "Protocol > |" << test.GetProtocol() << "|" << std::endl
			  << "Host > |" << test.GetHost() << "|" << std::endl;

	return 0;
}
