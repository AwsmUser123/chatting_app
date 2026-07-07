CMP=gcc
FLAGS=-O2 -Wall -Werror -Wextra -pedantic
LIBS=-lcurses

all: client server

utils.o:
	$(CMP) -c utils.c $(FLAGS) $(LIBS) -o utils.o

interface.o:
	$(CMP) -c interface.c $(FLAGS) $(LIBS) -o interface.o

client: interface.o utils.o
	$(CMP) client.c interface.o utils.o $(FLAGS) $(LIBS) -o client.out

server: utils.o
	$(CMP) server.c utils.o $(FLAGS) $(LIBS) -o server.out

clean:
	rm -f client.out server.out interface.o utils.o
