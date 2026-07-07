#ifndef __UTILS_H
#define __UTILS_H

void send_byte(int, char);

char recv_byte(int);

void send_confirm(int);

void send_str(int, char *);

void recv_str(int, char *);

void send_error(int, char *);

void handle_error(char *msg);

#endif
