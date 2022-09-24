ROOT = ./sources

NAME = webserv
CXX = clang++
CXXFLAGS = -Wall -Werror -Wextra -Iincludes -std=c++98 -pedantic
#-fsanitize=address -g3

SRC =	main.cpp \
		$(ROOT)/Config.cpp \
		$(ROOT)/ConfigMatcher.cpp \
		$(ROOT)/HttpInfo.cpp \
		$(ROOT)/HttpValidator.cpp \
		$(ROOT)/InputOutput.cpp \
		$(ROOT)/Data.cpp \
		$(ROOT)/Request.cpp \
		$(ROOT)/ConnectionHandler.cpp \
		$(ROOT)/CgiHandler.cpp \
		$(ROOT)/FileHandler.cpp \
		$(ROOT)/Monitor.cpp \
		$(ROOT)/FdTable.cpp \
		$(ROOT)/EventHandler.cpp \
		$(ROOT)/Processor.cpp \
		$(ROOT)/GetProcessor.cpp \
		$(ROOT)/DeleteProcessor.cpp \
		$(ROOT)/PostProcessor.cpp \
		$(ROOT)/UrlParser.cpp \
		$(ROOT)/utils.cpp \
		$(ROOT)/Response.cpp \
		$(ROOT)/Server.cpp

OBJ =	$(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

start:
	@sh server_content_setup.sh
	./webserv tests/tmp_config.json

config:
	$(CXX) $(CXXFLAGS) $(ROOT)/Config.cpp ./tests/config.cpp -o config

config_multi:
	$(CXX) $(CXXFLAGS) $(ROOT)/Config.cpp ./tests/multi_config.cpp -o config
	@touch sample_test_config.json
	@./config tests/multi_config.json sample_test_config.json
	@rm sample_test_config.json config

url_parser:
	$(CXX) $(CXXFLAGS) $(ROOT)/UrlParser.cpp ./tests/urlParser.cpp -o url_parser
	@./url_parser
	@rm url_parser

utils:
	$(CXX) $(CXXFLAGS) $(ROOT)/utils.cpp ./tests/utils.cpp -o utils
	@./utils
	@rm utils

clean:
	rm -rf $(OBJ)

fclean: clean
	rm -rf $(NAME)
	@rm -f config tests/tmp_config.json
	@rm -rf tests/www

re: fclean $(NAME)

.PHONY: all start config config_multi url_parser \
				utils clean fclean re
