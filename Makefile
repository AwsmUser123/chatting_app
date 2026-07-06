CMP=gcc
FLAGS=-O2 -Wall -Werror -Wextra -pedantic
LIBS=-lcurses

all: client server

interface.o:
	$(CMP) -c interface.c $(FLAGS) $(LIBS) -o interface.o

client: interface.o
	$(CMP) client.c interface.o $(FLAGS) $(LIBS) -o client.out

server:
	$(CMP) server.c $(FLAGS) $(LIBS) -o server.out

clean:
	rm -f client.out server.out interface.o
