#ifndef __INTERFACE_H
#define __INTERFACE_H

void initialize_ncurses();

void display_error(const char *);

void welcome_screen();

int account_screen();

void credentials_screen(char *, char *);

int loggedin_screen();

long join_chat();

int chats_screen();

void get_message(char *);

void list_messages(const char *);

void goodbye_screen();

#endif
