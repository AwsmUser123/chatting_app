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

void print_message(int);
void handle_error(char *);
void handle_response(int);
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
    handle_response(server);

    int running = 1;
    do {
        printf("What would you like to do?\n");
        printf("1) - Create an account.\n");
        printf("2) - Log into an account.\n");
        printf("3) - Create a new chat.\n");
        printf("4) - Join an existing chat.\n");
        printf("5) - List all existing chats.\n");
        printf("6) - Quit.\n");
        if (scanf("%d", &response) < 1)
            handle_error("Failed to read a response from user.\n");

        switch (response) {
            case 1: {
                // do account creation stuff
                int data_len;
                char account_name[NAME_LEN];
                char account_password[PASS_LEN];

                printf("Please enter your new account name: ");
                if (scanf("%s", account_name) < 1)
                    handle_error("Failed to read account name.\n");
                if (!credentials_valid(account_name))
                    handle_error("Invalid account name.\n");

                printf("Please enter your new password: ");
                if (scanf("%s", account_password) < 1)
                    handle_error("Failed to read password.\n");
                if (!credentials_valid(account_password))
                    handle_error("Invalid password.\n");
                buf[0] = 0x03;
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
                handle_response(server);
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
                handle_response(server);
                break;
            }
            case 3: {
                // do chat creation stuff
                buf[0] = 0x07;
                if (write(server, buf, 1) < 1)
                    handle_error("Failed to send data to the server.\n");
                print_message(server);
                handle_response(server);
                break;
            }
            case 4: {
                // do chat joining stuff
                buf[0] = 0x09;
                if (write(server, buf, 1) < 1)
                    handle_error("Failed to send data to the server.\n");
                long chat_id;
                printf("Please enter the ID of the chat you would like to join: ");
                if (scanf("%ld", &chat_id) < 1)
                    handle_error("Failed to read the ID of the chat from the user.\n");
                if (write(server, &chat_id, 4) < 4)
                    handle_error("Failed to send data to the server.\n");
                handle_response(server);
                break;
            }
            case 5: {
                // list all available chats
                buf[0] = 0x10;
                if (write(server, buf, 1) < 1)
                    handle_error("Failed to send data to the server.\n");
                print_message(server);
                break;
            }
            case 6: {
                // quit
                buf[0] = 0x20;
                if (write(server, buf, 1) < 1)
                    handle_error("Failed to send data to the server.\n");
                printf("Thank you for using the application.\n");
                running = 0;
                break;
            }
            default: {
                handle_error("Unknown code.\n");
            }
        }
    } while (running);

    return 0;
}

void print_message(int server_fd) {
    char buf[BUF_SIZE];
    int data_len;
    if (read(server_fd, &data_len, 4) < 4)
        handle_error("Failed to read the data from the server.\n");
    if (read(server_fd, buf, data_len) < data_len)
        handle_error("Failed to read the data from the server.\n");
    puts(buf);
}

void handle_error(char *msg) {
    if (errno != 0)
        perror(msg);
    else
        fprintf(stderr, "%s", msg);
    exit(EXIT_FAILURE);
}

void handle_response(int server_fd) {
    char buf[BUF_SIZE];
    if (read(server_fd, buf, 1) < 1)
        handle_error("Failed to read data from server.\n");
    if (buf[0] == 0x00) {
        int data_len;
        if (read(server_fd, &data_len, 4) < 4)
            handle_error("Failed to read data from server.\n");
        if (read(server_fd, buf, data_len) < data_len)
            handle_error("Failed to read data from server.\n");
        printf("ERROR: %s", buf);
    }
    else if (buf[0] == 0x02) {
        printf("OK!\n");
    }
    else
        handle_error("Unknown response code from server.\n");
}

int credentials_valid(char *str) {
    while (*str) {
        if (!isalnum(*str))
            return 0;
        str++;
    }
    return 1;
}
