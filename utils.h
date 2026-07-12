#ifndef __UTILS_H
#define __UTILS_H

typedef struct connection conn_t;

void send_byte(conn_t *, char);

char recv_byte(conn_t *);

void send_confirm(conn_t *);

void send_str(conn_t *, const char *);

void recv_str(conn_t *, char *);

void send_error(conn_t *, const char *);

void handle_error(const char *);

uint32_t htonl(uint32_t hostlong);

uint32_t ntohl(uint32_t netlong);

#endif
