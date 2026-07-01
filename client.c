#include <curses.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#define SERV_ADDR "192.168.2.186"
#define SERV_PORT 6879
#define QUEUE_LEN 5
#define BUF_SIZE 1024

void handle_error(char *);

int main() {
    struct sockaddr_in addr;
    int server;
    char buf[BUF_SIZE];
    int response;

    clear();
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
    
    printf("Welcome to the simple chatting application!\n");
    printf("Running the initial test...\n");

    buf[0] = 0x01;
    if (write(server, buf, 1) < 1)
        handle_error("Failed to send data to the server.\n");
    if (read(server, buf, 1) < 1)
        handle_error("Failed to receive data from the server.\n");
    if (buf[0] == 0x02)
        printf("OK!\n");
    else
        handle_error("Initial test failed. Exiting...\n");

    printf("What would you like to do?\n");
    printf("1) - Create an account.\n");
    printf("2) - Log into an account.\n");
    printf("3) - Quit.\n");
    if (scanf("%d", &response) < 1)
        handle_error("Failed to read a response from user.\n");

    switch (response) {
        case 1:
            // do account creation stuff
            break;
        case 2:
            // do login stuff
            break;
        case 3:
            buf[0] = 0x20;
            if (write(server, buf, 1) < 1)
                handle_error("Failed to send data to the server.\n");
            printf("Thank you for using the application.\n");
            break;
        default:
            handle_error("Unknown code.\n");
    }

    return 0;
}

void handle_error(char *msg) {
    if (errno != 0)
        perror(msg);
    else
        fprintf(stderr, "%s", msg);
    exit(EXIT_FAILURE);
}
