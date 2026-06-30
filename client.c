#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define SERV_ADDR "192.168.2.186"
#define SERV_PORT 6879
#define QUEUE_LEN 5

void handle_error(char *);

int main() {
    struct sockaddr_in addr;
    int server;
    char buf;

    buf = 0x00;
    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1) {
        handle_error("Socket creation failed.\n");
    }
    addr.sin_family = AF_INET;
    addr.sin_port = SERV_PORT;
    if (inet_aton(SERV_ADDR, &(addr.sin_addr)) == 0) {
        handle_error("Invalid server address.\n");
    }
    if (connect(server, (struct sockaddr *)(&addr), sizeof(addr)) == -1) {
        handle_error("Failed to connect to server.\n");
    }

    printf("\e[1;1H\e[2J");
    printf("Welcome to the simple chatting application!\n");
    printf("Running the initial test...\n");

    buf = 0x01;
    if (write(server, &buf, 1) < 1) {
        handle_error("Failed to send data to the server.\n");
    }
    if (read(server, &buf, 1) < 1) {
        handle_error("Failed to receive data from the server.\n");
    }
    if (buf == 0x02) {
        printf("OK!\n");
    }
    else {
        handle_error("Initial test failed. Exiting...\n");
    }

    return 0;
}

void handle_error(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}
