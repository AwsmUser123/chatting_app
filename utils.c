#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codes.h"
#include "constants.h"
#include "netutils.h"
#include "utils.h"

#if defined(_WIN32)
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

void send_byte(conn_t *connection, char byte) {
    if (write_conn(connection, &byte, 1) == -1)
        handle_error("Failed to send data to remote destination.\n");
}

char recv_byte(conn_t *connection) {
    char res;
    if (read_conn(connection, &res, 1) == -1)
        handle_error("Failed to read data from remote source.\n");
    return res;
}

void send_confirm(conn_t *connection) {
    send_byte(connection, CMD_OK);
} 

void send_str(conn_t *connection, const char *src) {
    size_t data_len = strlen(src)+1;
    uint32_t serialized_len = htonl(data_len);
    if (write_conn(connection, &serialized_len, sizeof(size_t)) == -1)
        handle_error("Failed to send the data to the remote destination.\n");
    if (write_conn(connection, src, data_len) == -1)
        handle_error("Failed to send the data to the remote destination.\n");
}

void recv_str(conn_t *connection, char *dst) {
    size_t data_len;
    if (read_conn(connection, &data_len, sizeof(size_t)) == -1)
        handle_error("Failed to read data from remote source.\n");
    data_len = ntohl(data_len);
    if (read_conn(connection, dst, data_len) == -1)
        handle_error("Failed to read data from remote source.\n");
}

void send_error(conn_t *connection, const char *msg) {
    char buf[BUF_SIZE];
    if (errno != 0)
        snprintf(buf, BUF_SIZE, "%s:%s", msg, strerror(errno));
    else
        snprintf(buf, BUF_SIZE, "%s", msg);
    send_byte(connection, CMD_ERROR);
    send_str(connection, buf);
}
