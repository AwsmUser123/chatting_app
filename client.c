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

void send_code(int, char);
void send_message(int, char *);
void print_message(int);
void handle_error(char *);
int handle_response(int);
int credentials_valid(char *);

void initialize_ncurses();
void welcome_screen();
int login_screen();
int join_screen();
int chat_screen();

int main() {
    struct sockaddr_in addr;
    int server;
    char buf[BUF_SIZE];
    int response;
    enum state {
        LOGGED_OUT,
        LOGGED_IN,
        IN_CHAT
    } current_state = LOGGED_OUT;

    initialize_ncurses();

    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1) {
        handle_error("Socket creation failed.\n");
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERV_PORT);
    if (inet_aton(SERV_ADDR, &(addr.sin_addr)) == 0) {
        handle_error("Invalid server address.\n");
    }
    if (connect(server, (struct sockaddr *)(&addr), sizeof(addr)) == -1) {
        handle_error("Failed to connect to server.\n");
    }

    printw("Welcome to the simple chatting application!\n");
    printw("Running the initial test...\n");

    send_code(server, 0x01);
    handle_response(server);

    int running = 1;
    while (running) {
        clear();
        refresh();
        switch(current_state) {
            case LOGGED_OUT: {
                printw("What would you like to do?\n");
                printw("1) - Create an account.\n");
                printw("2) - Log into an account.\n");
                printw("3) - Quit.\n");
                if (scanf("%d", &response) < 1)
                    handle_error("Failed to read a response from user.\n");

                switch (response) {
                    case 1: {
                        char account_name[NAME_LEN];
                        char account_password[PASS_LEN];

                        printw("Please enter your new account name: ");
                        if (scanf("%s", account_name) < 1)
                            handle_error("Failed to read account name.\n");
                        if (!credentials_valid(account_name))
                            handle_error("Invalid account name.\n");

                        printw("Please enter your new password: ");
                        if (scanf("%s", account_password) < 1)
                            handle_error("Failed to read password.\n");
                        if (!credentials_valid(account_password))
                            handle_error("Invalid password.\n");

                        strcpy(buf, "");
                        strcat(buf, account_name);
                        strcat(buf, ":");
                        strcat(buf, account_password);

                        send_code(server, 0x03);
                        send_message(server, buf);
                        if (handle_response(server) == 0)
                            current_state = LOGGED_IN;
                        break;
                    }
                    case 2: {
                        char account_name[NAME_LEN];
                        char account_password[PASS_LEN];

                        printw("Please enter your account name: ");
                        if (scanf("%s", account_name) < 1)
                            handle_error("Failed to read account name.\n");
                        if (!credentials_valid(account_name))
                            handle_error("Invalid account name.\n");

                        printw("Please enter your password: ");
                        if (scanf("%s", account_password) < 1)
                            handle_error("Failed to read password.\n");
                        if (!credentials_valid(account_password))
                            handle_error("Invalid password.\n");

                        strcpy(buf, "");
                        strcat(buf, account_name);
                        strcat(buf, ":");
                        strcat(buf, account_password);

                        send_code(server, 0x05);
                        send_message(server, buf);
                        if (handle_response(server) == 0)
                            current_state = LOGGED_IN;
                        break;
                    }
                    case 3: {
                        send_code(server, 0x20);
                        running = 0;
                        break;
                    }
                    default: {
                        printw("Unknown code.\n");
                    }
                }
                break;
            }
            case LOGGED_IN: {
                printw("What would you like to do?\n");
                printw("1) - Create a chat.\n");
                printw("2) - Join a chat.\n");
                printw("3) - List all chats.\n");
                printw("4) - Log out.\n");
                printw("5) - Quit.\n");
                if (scanf("%d", &response) < 1)
                    handle_error("Failed to read a response from user.\n");

                switch (response) {
                    case 1: {
                        send_code(server, 0x07);
                        print_message(server);
                        if (handle_response(server) == 0)
                            current_state = IN_CHAT;
                        break;
                    }
                    case 2: {
                        send_code(server, 0x09);
                        long chat_id;
                        printw("Please enter the ID of the chat you would like to join: ");
                        if (scanf("%ld", &chat_id) < 1)
                            handle_error("Failed to read the ID of the chat from the user.\n");
                        if (write(server, &chat_id, 8) < 8)
                            handle_error("Failed to send data to the server.\n");
                        if (handle_response(server) == 0)
                            current_state = IN_CHAT;
                        break;
                    }
                    case 3: {
                        send_code(server, 0x10);
                        print_message(server);
                        break;
                    }
                    case 4: {
                        send_code(server, 0x06);
                        if (handle_response(server) == 0)
                            current_state = LOGGED_OUT;
                        break;
                    }
                    case 5: {
                        send_code(server, 0x06);
                        handle_response(server);
                        send_code(server, 0x20);
                        running = 0;
                        break;
                    }
                    default: {
                        printw("Unknown code.\n");
                    }
                }
                break;
            }
            case IN_CHAT: {
                printw("What would you like to do?\n");
                printw("1) - Send a message.\n");
                printw("2) - List all messages.\n");
                printw("3) - Leave chat.\n");
                printw("4) - Quit.\n");
                if (scanf("%d", &response) < 1)
                    handle_error("Failed to read a response from user.\n");

                switch (response) {
                    case 1: {
                        send_code(server, 0x0b);
                        printw("Please enter your message:\n");
                        while (getchar() != '\n');
                        if (fgets(buf, BUF_SIZE, stdin) == NULL)
                            handle_error("Failed to read the message from the user.\n");
                        send_message(server, buf);
                        handle_response(server);
                        break;
                    }
                    case 2: {
                        send_code(server, 0x0d);
                        print_message(server);
                        break;
                    }
                    case 3: {
                        send_code(server, 0x0a);
                        if (handle_response(server) == 0)
                            current_state = LOGGED_IN;
                        break;
                    }
                    case 4: {
                        send_code(server, 0x0a);
                        handle_response(server);
                        send_code(server, 0x06);
                        handle_response(server);
                        send_code(server, 0x20);
                        running = 0;
                        break;
                    }
                    default: {
                        printw("Unknown code.\n");
                    }
                }
                break;
            }
        }
    }
    printw("Thank you for using the chatting application!\n");
    endwin();

    return 0;
}

void send_code(int server_fd, char code) {
    if (write(server_fd, &code, 1) < 1)
        handle_error("Failed to send data to the server.\n");
}

void send_message(int server_fd, char *msg) {
    int data_len;
    data_len = strlen(msg)+1;
    if (write(server_fd, &data_len, 4) < 4)
        handle_error("Failed to send the data to the server.\n");
    if (write(server_fd, msg, data_len) < data_len)
        handle_error("Failed to send the data to the server.\n");
}

void print_message(int server_fd) {
    char buf[BUF_SIZE];
    int data_len = 0;
    int res;
    if ((res = read(server_fd, &data_len, 4)) < 4) {
        handle_error("Failed to read the data from the server.\n");
    }
    if (read(server_fd, buf, data_len) < data_len)
        handle_error("Failed to read the data from the server.\n");
    printw("%s", buf);
}

void handle_error(char *msg) {
    if (errno != 0)
        printw("%s\n\t%s", msg, strerror(errno));
    else
        printw("%s", msg);
    endwin();
    exit(EXIT_FAILURE);
}

int handle_response(int server_fd) {
    char buf[BUF_SIZE];
    if (read(server_fd, buf, 1) < 1)
        handle_error("Failed to read data from server.\n");
    if (buf[0] == 0x00) {
        int data_len;
        if (read(server_fd, &data_len, 4) < 4)
            handle_error("Failed to read data from server.\n");
        if (read(server_fd, buf, data_len) < data_len)
            handle_error("Failed to read data from server.\n");
        printw("ERROR: %s", buf);
        return 1;
    }
    else if (buf[0] == 0x02) {
        return 0;
    }
    else {
        handle_error("Unknown response code from server.\n");
        return -1;
    }
}

int credentials_valid(char *str) {
    while (*str) {
        if (!isalnum(*str))
            return 0;
        str++;
    }
    return 1;
}

void initialize_ncurses() {
    initscr();
    cbreak();
    noecho();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);
}

void welcome_screen();
int login_screen();
int join_screen();
int chat_screen();
