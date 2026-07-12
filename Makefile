CMP=gcc
FLAGS=-g -O2 -Wall -Werror -Wextra -pedantic
LIBS=-lncurses

ifeq ($(OS),Windows_NT)
    PLATFFORM := Windows
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        PLATFORM := Linux
    else
        PLATFORM := Unknown
    endif
endif

all: client server

netutils.o:
	@if [ "$(PLATFORM)" = "Windows" ]; then \
		$(CMP) -c netutils_win.c $(FLAGS) $(LIBS) -o netutils.o; \
	elif [ "$(PLATFORM)" = "Linux" ]; then \
        $(CMP) -c netutils_unix.c $(FLAGS) $(LIBS) -o netutils.o; \
	fi

utils.o:
	$(CMP) -c utils.c $(FLAGS) $(LIBS) -o utils.o

interface.o:
	$(CMP) -c interface.c $(FLAGS) $(LIBS) -o interface.o

client: interface.o netutils.o utils.o
	$(CMP) client.c $^ $(FLAGS) $(LIBS) -o client.out

server: netutils.o utils.o
	$(CMP) server.c $^ $(FLAGS) $(LIBS) -o server.out

clean:
	rm -f *.o *.out
