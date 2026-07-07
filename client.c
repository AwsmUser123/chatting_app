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

#include "utils.h"
#include "interface.h"

#define SERV_ADDR "192.168.2.186"
#define SERV_PORT 6879
#define QUEUE_LEN 5
#define BUF_SIZE 1024
#define NAME_LEN 128
#define PASS_LEN 128

int handle_response(int, char *);

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
    welcome_screen();

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

    send_byte(server, 0x01);
    handle_response(server, buf);

    int running = 1;
    while (running) {
        switch(current_state) {
            case LOGGED_OUT: {
                response = account_screen();

                switch (response) {
                    case 1: {
                        char account_name[NAME_LEN];
                        char account_password[PASS_LEN];

                        credentials_screen(account_name, account_password);

                        strcpy(buf, "");
                        strcat(buf, account_name);
                        strcat(buf, ":");
                        strcat(buf, account_password);

                        send_byte(server, 0x03);
                        send_str(server, buf);
                        if (handle_response(server, buf) == 0)
                            current_state = LOGGED_IN;
                        break;
                    }
                    case 2: {
                        char account_name[NAME_LEN];
                        char account_password[PASS_LEN];

                        credentials_screen(account_name, account_password);

                        strcpy(buf, "");
                        strcat(buf, account_name);
                        strcat(buf, ":");
                        strcat(buf, account_password);

                        send_byte(server, 0x05);
                        send_str(server, buf);
                        if (handle_response(server, buf) == 0)
                            current_state = LOGGED_IN;
                        break;
                    }
                    case 3: {
                        send_byte(server, 0x20);
                        running = 0;
                        break;
                    }
                }
                break;
            }
            case LOGGED_IN: {
                response = loggedin_screen();

                switch (response) {
                    case 1: {
                        send_byte(server, 0x07);
                        if (handle_response(server, buf) == 0)
                            current_state = IN_CHAT;
                        break;
                    }
                    case 2: {
                        send_byte(server, 0x09);
                        long chat_id;
                        chat_id = join_chat();
                        if (write(server, &chat_id, 8) < 8)
                            handle_error("Failed to send data to the server.\n");
                        if (handle_response(server, buf) == 0)
                            current_state = IN_CHAT;
                        break;
                    }
                    case 3: {
                        send_byte(server, 0x10);
                        recv_str(server, buf);
                        list_messages(buf);
                        break;
                    }
                    case 4: {
                        send_byte(server, 0x06);
                        if (handle_response(server, buf) == 0)
                            current_state = LOGGED_OUT;
                        break;
                    }
                    case 5: {
                        send_byte(server, 0x06);
                        handle_response(server, buf);
                        send_byte(server, 0x20);
                        running = 0;
                        break;
                    }
                }
                break;
            }
            case IN_CHAT: {
                response = chats_screen();

                switch (response) {
                    case 1: {
                        send_byte(server, 0x0b);
                        get_message(buf);
                        send_str(server, buf);
                        handle_response(server, buf);
                        break;
                    }
                    case 2: {
                        send_byte(server, 0x0d);
                        recv_str(server, buf);
                        list_messages(buf);
                        break;
                    }
                    case 3: {
                        send_byte(server, 0x0a);
                        if (handle_response(server, buf) == 0)
                            current_state = LOGGED_IN;
                        break;
                    }
                    case 4: {
                        send_byte(server, 0x0a);
                        handle_response(server, buf);
                        send_byte(server, 0x06);
                        handle_response(server, buf);
                        send_byte(server, 0x20);
                        running = 0;
                        break;
                    }
                }
                break;
            }
        }
    }

    goodbye_screen();
    return 0;
}

int handle_response(int server_fd, char *err_str) {
    switch (recv_byte(server_fd)) {
        case 0x00: {
            recv_str(server_fd, err_str);
            display_error(err_str);
            return 1;
        }
        case 0x02: {
            return 0;
        }
        default: {
            handle_error("Unknown response code from server.\n");
            return -1;
        }
    }
}
