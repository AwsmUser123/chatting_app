#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h> 

#include "constants.h"
#include "netutils.h"

struct connection {
    int sockfd;
    struct sockaddr_in address;
    socklen_t addrlen;
    bool use_tls;
};

void init_net() {
    return;
}

void end_net() {
    return;
}

conn_t *create_client_conn(const char *ip_string, const char *port) {
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
    addr.sin_port = htons(atoi(port));
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

conn_t *create_server_conn(const char *ip_string, const char *port) {
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
    addr.sin_port = htons(atoi(port));
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

    client->sockfd = -1;
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

void close_conn(conn_t *connection) {
    if (connection == NULL || connection->sockfd < 0)
        return;
    close(connection->sockfd);
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
        ssize_t received = recv(connection->sockfd, p, count, 0);
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
        ssize_t written = send(connection->sockfd, p, count, 0);
        if (written == -1 && errno == EINTR)
            continue;
        else if (written <= 0)
            return -1;
        p += written;
        count -= written;
    }
    return 0;
}
