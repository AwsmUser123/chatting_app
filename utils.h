#ifndef __UTILS_H
#define __UTILS_H

void send_byte(int, char);

char recv_byte(int);

void send_u64(int, uint64_t);

uint64_t recv_u64(int);

void send_confirm(int);

void send_str(int, const char *);

void recv_str(int, char *);

void send_error(int, const char *);

void handle_error(const char *);

#endif
