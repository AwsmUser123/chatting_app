#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "utils.h"

#define BUF_SIZE 1024

void send_byte(int fd, char byte) {
    if (write(fd, &byte, 1) < 1)
        handle_error("Failed to send data to remote destination.\n");
}

char recv_byte(int fd) {
    char res;
    if (read(fd, &res, 1) < 1)
        handle_error("Failed to read data from remote source.\n");
    return res;
}

void send_confirm(int fd) {
    send_byte(fd, 0x02);
} 

void send_str(int fd, const char *src) {
    int data_len;
    data_len = strlen(src)+1;
    if (write(fd, &data_len, 4) < 4)
        handle_error("Failed to send the data to the client.\n");
    if (write(fd, src, data_len) < data_len)
        handle_error("Failed to send the data to the client.\n");
}

void recv_str(int fd, char *dst) {
    int data_len;
    if (read(fd, &data_len, 4) < 4)
        handle_error("Failed to read the data from the server.\n");
    if (read(fd, dst, data_len) < data_len)
        handle_error("Failed to read the data from the server.\n");
}

void send_error(int fd, const char *msg) {
    char buf[BUF_SIZE];
    if (errno != 0)
        sprintf(buf, "%s:%s", msg, strerror(errno));
    else
        strcpy(buf, msg);
    send_byte(fd, 0x00);
    send_str(fd, buf);
}

void handle_error(const char *msg) {
    char buf[BUF_SIZE];
    strcpy(buf, msg);
    if (errno != 0) {
        strcat(buf, "\n");
        strcat(buf, strerror(errno));
        errno = 0;
    }
#ifdef __INTERFACE_H
    display_error(buf);
    endwin();
#else
    fputs(buf, stderr);
#endif
    exit(EXIT_FAILURE);
}
