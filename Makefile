CMP=gcc
FLAGS=-O2 -Wall -Werror -Wextra -pedantic
LIBS=-lcurses

server:
	$(CMP) server.c $(FLAGS) $(LIBS) -o server.out

client:
	$(CMP) client.c $(FLAGS) $(LIBS) -o client.out
