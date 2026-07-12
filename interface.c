#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "interface.h"

void initialize_ncurses() {
    printf("Initializing...\n");
}

void end_ncurses() {
    printf("Finishing..\n");
}

void display_error(const char *str) {
    printf("ERROR:\n");
    puts(str);
}

void welcome_screen() {
    printf("Welcome to the chatting application!\n");
    printf("Press ENTER to continue.\n");
    getchar();
}

int account_screen() {
    int selected;
    printf("What would you like to do?\n");
    printf("1) - Create an account.\n");
    printf("2) - Log into an account.\n");
    printf("3) - Quit.\n");
    if (scanf("%d", &selected) < 1)
        selected = 3;
    if (selected < 1 || selected > 3)
        selected = 3;
    return selected;
}

void credentials_screen(char *username, char *password) {
    printf("USER DATA FORM\n");
    printf("Enter your username:\n");
    while (getchar() != '\n');
    if (scanf("%s", username) < 1)
        printf("Failed to read username from user.\n");
    printf("Enter your password:\n");
    while (getchar() != '\n');
    if (scanf("%s", password) < 1)
        printf("Failed to read password from user.\n");
}

int loggedin_screen() {
    int selected;
    printf("What would you like to do?\n");
    printf("1) - Create a chat.\n");
    printf("2) - Join a chat.\n");
    printf("3) - List all chats.\n");
    printf("4) - Log out.\n");
    printf("5) - Quit.\n");
    if (scanf("%d", &selected) < 1)
        selected = 5;
    if (selected < 1 || selected > 5)
        selected = 5;
    return selected;
}

void join_chat(char *chat_id) {
    printf("CHAT JOIN\n");
    printf("Please enter the ID of the chat you would like to join:\n");
    while (getchar() != '\n');
    if (scanf("%s", chat_id) < 1)
        printf("Failed to read chat ID from user.\n");
}

int chats_screen() {
    int selected;
    printf("What would you like to do?\n");
    printf("1) - Send a message.\n");
    printf("2) - List all messages.\n");
    printf("3) - Leave chat.\n");
    printf("4) - Quit.\n");
    if (scanf("%d", &selected) < 1)
        selected = 4;
    if (selected < 1 || selected > 4)
        selected = 4;
    return selected;
}

void get_message(char *message) {
    while (getchar() != '\n');
    if (fgets(message, BUF_SIZE-1, stdin) == NULL)
        printf("Failed to read message from user.\n");
}

void list_messages(const char *str) {
    putchar('\n');
    puts(str);
    putchar('\n');
}

void goodbye_screen() {
    printf("Thank you for using this application!\n");
    getchar();
}
