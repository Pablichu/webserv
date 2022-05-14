ROOT = ./sources

NAME = webserv
CXX = g++
CXXFLAGS = -Wall -Werror -Wextra -Iincludes -fsanitize=address -g3 -std=c++98 -pedantic

SRC =	main.cpp \
		$(ROOT)/Config.cpp \
		$(ROOT)/Request.cpp \
		$(ROOT)/Response.cpp \
		$(ROOT)/CgiResponse.cpp \
		$(ROOT)/Monitor.cpp \
		$(ROOT)/Server.cpp

OBJ =	$(SRC:.cpp=.o)

MSG = Default commit message
#MSG="message" to change message


all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

response:
	$(CXX) $(CXXFLAGS) $(ROOT)/Response.cpp ./tests/response.cpp -o response

request:
	$(CXX) $(CXXFLAGS) $(ROOT)/Request.cpp ./tests/requestTest.cpp -o request

config:
	$(CXX) $(CXXFLAGS) $(ROOT)/Config.cpp ./tests/config.cpp -o config

config_multi:
	$(CXX) $(CXXFLAGS) $(ROOT)/Config.cpp ./tests/multi_config.cpp -o config
	@touch sample_test_config.json
	@./config tests/multi_config.json sample_test_config.json
	@rm sample_test_config.json config

monitor:
	$(CXX) $(CXXFLAGS) $(ROOT)/Monitor.cpp ./tests/monitor.cpp -o monitor

clean:
	rm -rf $(NAME)

fclean: clean
	rm -rf $(OBJ)

re: fclean $(NAME)

git: fclean
	git add .
	git commit -m "$(MSG)"
	git push

.PHONY: all response request config config_multi monitor clean fclean re
