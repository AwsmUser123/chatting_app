CMP=gcc
FLAGS=-O2 -Wall -Werror -Wextra -pedantic
LIBS=-lcurses

all: client server

client:
	$(CMP) client.c $(FLAGS) $(LIBS) -o client.out

server:
	$(CMP) server.c $(FLAGS) $(LIBS) -o server.out

clean:
	rm -f client.out server.out
