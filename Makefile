ROOT = ./sources

NAME = webserv
CXX = g++
CXXFLAGS = -Wall -Werror -Wextra -Iincludes -fsanitize=address -g3 -std=c++98 -pedantic

SRC =	main.cpp \
		$(ROOT)/Config.cpp \
		$(ROOT)/Petition.cpp \
		$(ROOT)/Response.cpp \
		$(ROOT)/Server.cpp

OBJ =	$(SRC:.cpp=.o)

MSG = Default commit message
#MSG="message" to change message


all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

response:
	$(CXX) $(CXXFLAGS) $(ROOT)/Response.cpp ./tests/response.cpp -o response

config:
	$(CXX) $(CXXFLAGS) $(ROOT)/Config.cpp ./tests/config.cpp -o config

config_multi:
	$(CXX) $(CXXFLAGS) $(ROOT)/Config.cpp ./tests/multi_config.cpp -o config
	@touch sample_test_config.json
	@./config tests/multi_config.json sample_test_config.json
	@rm sample_test_config.json config

clean:
	rm -rf $(NAME)

fclean: clean
	rm -rf $(OBJ)

re: fclean $(NAME)

git: fclean
	git add .
	git commit -m "$(MSG)"
	git push
