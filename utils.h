#ifndef __UTILS_H
#define __UTILS_H

#include <sys/socket.h>
#include <netinet/in.h>

typedef struct connection {
    int sockfd;
    struct sockaddr_in address;
    socklen_t addrlen;
    bool use_tls;
} conn_t;

void handle_error(const char *);

conn_t *create_client_conn(const char *, short);

conn_t *create_server_conn(const char *, short);

conn_t *create_accept_conn();

void accept_client_conn(conn_t *, conn_t *);

conn_t *free_conn(conn_t *);

void send_byte(conn_t *, char);

char recv_byte(conn_t *);

void send_confirm(conn_t *);

void send_str(conn_t *, const char *);

void recv_str(conn_t *, char *);

void send_error(conn_t *, const char *);

void handle_error(const char *);

#endif
