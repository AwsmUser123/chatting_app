#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h> //unix?
#include <sys/socket.h> //unix

#include "codes.h"
#include "constants.h"
#include "utils.h"

conn_t *create_client_conn(const char *ip_string, short port) {
    struct sockaddr_in addr;
    socklen_t addrlen;

    conn_t *server = malloc(sizeof(conn_t));
    if (server == NULL)
        handle_error("Failed to allocate memory.\n");

    server->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->sockfd == -1) {
        free(server);
        handle_error("Socket creation failed.\n");
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_aton(ip_string, &(addr.sin_addr)) == 0) {
        free(server);
        handle_error("Invalid server address.\n");
    }

    addrlen = sizeof(addr);
    server->address = addr;
    server->addrlen = addrlen;
    server->use_tls = false;

    if (connect(server->sockfd, (struct sockaddr *)(&addr), addrlen) == -1) {
        free(server);
        handle_error("Failed to connect to server.\n");
    }
    return server;
}

conn_t *create_server_conn(const char *ip_string, short port) {
    struct sockaddr_in addr;
    socklen_t addrlen;

    conn_t *server = malloc(sizeof(conn_t));
    if (server == NULL)
        handle_error("Failed to allocate memory.\n");

    server->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->sockfd == -1) {
        free(server);
        handle_error("Socket creation failed.\n");
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_aton(ip_string, &(addr.sin_addr)) == 0) {
        free(server);
        handle_error("Invalid server address.\n");
    }

    addrlen = sizeof(addr);
    server->address = addr;
    server->addrlen = addrlen;
    server->use_tls = false;

    if (bind(server->sockfd, (struct sockaddr *)(&addr), addrlen) == -1) {
        free(server);
        handle_error("Failed to bind the address to the socket.\n");
    }
    if (listen(server->sockfd, QUEUE_LEN) == -1) {
        free(server);
        handle_error("Failed to mark the socket as listening.\n");
    }
    return server;
}

conn_t *create_accept_conn() {
    conn_t *client = malloc(sizeof(conn_t));
    if (client == NULL)
        handle_error("Failed to allocate memory.\n");

    client->addrlen = sizeof(struct sockaddr_in);
    client->use_tls = false;
    return client;
}

void accept_client_conn(conn_t *client, conn_t *server) {
    client->sockfd = accept(server->sockfd, (struct sockaddr *)&(client->address), &(client->addrlen));
    if (client->sockfd == -1) {
        free(client);
        handle_error("Failed to accept a client connection.\n");
    }
}

conn_t *free_conn(conn_t *connection) {
    if (connection != NULL)
        free(connection);
    return NULL;
}

ssize_t read_conn(conn_t *connection, void *buf, size_t count) {
    if (connection == NULL)
        return -1;
    char *p = buf;
    while (count > 0) {
        ssize_t received = read(connection->sockfd, p, count);
        if (received == -1 && errno == EINTR)
            continue;
        else if (received <= 0)
            return -1;
        p += received;
        count -= received;
    }
    return 0;
}

ssize_t write_conn(conn_t *connection, const void *buf, size_t count) {
    if (connection == NULL)
        return -1;
    const char *p = buf;
    while (count > 0) {
        ssize_t written = write(connection->sockfd, p, count);
        if (written == -1 && errno == EINTR)
            continue;
        else if (written <= 0)
            return -1;
        p += written;
        count -= written;
    }
    return 0;
}

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
