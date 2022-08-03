#include "webserv.hpp"

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

void	pruebaPruebita(Request & req)
{
	std::string & test1 = req.getData(0);
	std::string & test2 = req.getData(0);

	std::string coso = "Prueba pruebita";
	test1 = coso;
	std::cout << "_____ test _____" << std::endl
			  << test2 << std::endl;
	coso = "Segunda pruebita";
	test1 = coso;
	std::cout << "_____ test _____" << std::endl
			  << test2 << std::endl;

	std::string & test3 = req.getData(0);
	coso = "Tercera pruebita";
	test1 = coso;
	std::cout << "_____ test _____" << std::endl
			  << test3 << std::endl;

	coso = "Cuarta pruebita";
	test1 = coso;
	test1.append(": Movida movidota esta mierda está hechota");
	test1.append(" eeh? KE LO KE");
	std::cout << "_____ test _____" << std::endl
			  << test2 << std::endl;
	return ;
}

int	main()
{
	//std::map<std::string, std::string>::iterator  it;
	Request                                        req;

/*	std::string header = getFileInString();
	std::cout << "_____ txt EXAMPLE _____" << std::endl << header << std::endl;

	req.process(header);
	std::cout << std::endl << "_____ Test with Map _____" << std::endl;

	for (it = req.begin(); it != req.end(); it++) {
		std::cout << it->first << " = " << it->second << std::endl;
	}*/
	
	pruebaPruebita(req);
	std::string & test = req.getData(0);
	std::cout << "_____ Final test _____" << std::endl
			  << test << std::endl;
	return 0;
}
