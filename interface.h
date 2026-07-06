#ifndef __INTERFACE
#define __INTERFACE

void initialize_ncurses();

void display_error(char *str);

void welcome_screen();

int account_screen();

void credentials_screen(char *, char *);

int loggedin_screen();

long join_chat();

int chats_screen();

void get_message(char *message);

void list_messages(char *str);

void goodbye_screen();

#endif
