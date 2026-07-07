#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "utils.h"

#define BUF_SIZE 1024

void send_byte(int server_fd, char code) {
    if (write(server_fd, &code, 1) < 1)
        handle_error("Failed to send data to remote destination.\n");
}

char recv_byte(int server_fd) {
    char res;
    if (read(server_fd, &res, 1) < 1)
        handle_error("Failed to read data from remote source.\n");
    return res;
}

void send_confirm(int client_fd) {
    send_byte(client_fd, 0x02);
} 

void send_str(int client_fd, char *src) {
    int data_len;
    data_len = strlen(src)+1;
    if (write(client_fd, &data_len, 4) < 4)
        handle_error("Failed to send the data to the client.\n");
    if (write(client_fd, src, data_len) < data_len)
        handle_error("Failed to send the data to the client.\n");
}

void recv_str(int client_fd, char *dst) {
    int data_len;
    if (read(client_fd, &data_len, 4) < 4)
        handle_error("Failed to read the data from the server.\n");
    if (read(client_fd, dst, data_len) < data_len)
        handle_error("Failed to read the data from the server.\n");
}

void send_error(int client_fd, char *msg) {
    char buf[BUF_SIZE];
    if (errno != 0)
        sprintf(buf, "%s:%s", msg, strerror(errno));
    else
        sprintf(buf, "%s", msg);
    send_byte(client_fd, 0x00);
    send_str(client_fd, msg);
}

void handle_error(char *msg) {
    char buf[BUF_SIZE];
    strcpy(buf, msg);
    if (errno != 0) {
        strcat(buf, "\n");
        strcat(buf, strerror(errno));
    }
#ifdef __INTERFACE_H
    display_error(buf);
    endwin();
#else
    fputs(buf, stderr);
#endif
    exit(EXIT_FAILURE);
}
