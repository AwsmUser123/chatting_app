#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "codes.h"
#include "constants.h"
#include "utils.h"

ssize_t read_all(int fd, void *buf, size_t count) {
    char *p = buf;
    while (count > 0) {
        ssize_t received = read(fd, p, count);
        if (received == -1 && errno == EINTR)
            continue;
        else if (received <= 0)
            return -1;
        p += received;
        count -= received;
    }
    return 0; //return count
}

ssize_t write_all(int fd, const void *buf, size_t count) {
    const char *p = buf;
    while (count > 0) {
        ssize_t written = write(fd, p, count);
        if (written == -1 && errno == EINTR)
            continue;
        else if (written <= 0)
            return -1;
        p += written;
        count -= written;
    }
    return 0; //return count
}

void send_byte(int fd, char byte) {
    if (write_all(fd, &byte, 1) == -1)
        handle_error("Failed to send data to remote destination.\n");
}

char recv_byte(int fd) {
    char res;
    if (read_all(fd, &res, 1) == -1)
        handle_error("Failed to read data from remote source.\n");
    return res;
}

void send_u64(int fd, uint64_t data) {
    if (write_all(fd, &data, 8) == -1)
        handle_error("Failed to send data to remote destination.\n");
}

uint64_t recv_u64(int fd) {
    uint64_t res;
    if (read_all(fd, &res, 8) == -1)
        handle_error("Failed to read data from remote source.\n");
    return res;
}

void send_confirm(int fd) {
    send_byte(fd, CMD_OK);
} 

void send_str(int fd, const char *src) {
    int data_len;
    data_len = strlen(src)+1;
    if (write_all(fd, &data_len, 4) == -1)
        handle_error("Failed to send the data to the client.\n");
    if (write_all(fd, src, data_len) == -1)
        handle_error("Failed to send the data to the client.\n");
}

void recv_str(int fd, char *dst) {
    int data_len;
    if (read_all(fd, &data_len, 4) == -1)
        handle_error("Failed to read the data from the server.\n");
    if (read_all(fd, dst, data_len) == -1)
        handle_error("Failed to read the data from the server.\n");
}

void send_error(int fd, const char *msg) {
    char buf[BUF_SIZE];
    if (errno != 0)
        snprintf(buf, BUF_SIZE, "%s:%s", msg, strerror(errno));
    else
        snprintf(buf, BUF_SIZE, "%s", msg);
    send_byte(fd, CMD_ERROR);
    send_str(fd, buf);
}
