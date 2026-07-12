#ifndef __NETUTILS_H
#define __NETUTILS_H

typedef struct connection conn_t;

void init_net();

void end_net();

conn_t *create_client_conn(const char *ip_string, const char *port);

conn_t *create_server_conn(const char *ip_string, const char *port);

conn_t *create_accept_conn();

void accept_client_conn(conn_t *client, conn_t *server);

void close_conn(conn_t *connection);

conn_t *free_conn(conn_t *connection);

ssize_t read_conn(conn_t *connection, void *buf, size_t count);

ssize_t write_conn(conn_t *connection, const void *buf, size_t count);

void handle_error(const char *);

#endif
