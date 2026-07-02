#include <ctype.h>
#include <curses.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#define SERV_ADDR "192.168.2.186"
#define SERV_PORT 6879
#define QUEUE_LEN 5
#define BUF_SIZE 1024
#define NAME_LEN 128
#define PASS_LEN 128

void handle_error(char *);
int credentials_valid(char *);

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
        case 1: {
            // do account creation stuff
            break;
        }
        case 2: {
            // do login stuff
            int data_len;
            char account_name[NAME_LEN];
            char account_password[PASS_LEN];

            printf("Please enter your account name: ");
            if (scanf("%s", account_name) < 1)
                handle_error("Failed to read account name.\n");
            if (!credentials_valid(account_name))
                handle_error("Invalid account name.\n");

            printf("Please enter your password: ");
            if (scanf("%s", account_password) < 1)
                handle_error("Failed to read password.\n");
            if (!credentials_valid(account_password))
                handle_error("Invalid password.\n");
            buf[0] = 0x05;
            data_len = strlen(account_name) + strlen(account_password) + 1;
            strcpy(buf+1, account_name);
            buf[strlen(account_name)+1] = ':';
            strcpy(buf+strlen(account_name)+2, account_password);
            if (write(server, buf, 1) < 1)
                handle_error("Failed to send data to the server.\n");
            if (write(server, &data_len, 4) < 4)
                handle_error("Failed to send data to the server.\n");
            if (write(server, buf+1, data_len) < data_len)
                handle_error("Failed to send data to the server.\n");
            if (read(server, buf, 1) < 1)
                handle_error("Failed to receive data from the server.\n");
            if (buf[0] == 0x06)
                printf("Logged in!\n");
            else
                handle_error("Login failed.\n");
            break;
        }
        case 3: {
            // quit
            buf[0] = 0x20;
            if (write(server, buf, 1) < 1)
                handle_error("Failed to send data to the server.\n");
            printf("Thank you for using the application.\n");
            break;
        }
        default: {
            handle_error("Unknown code.\n");
        }
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

int credentials_valid(char *str) {
    while (*str) {
        if (!isalnum(*str))
            return 0;
        str++;
    }
    return 1;
}
