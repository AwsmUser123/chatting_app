#include <ctype.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "interface.h"

#define max(a, b) (((a) > (b)) ? (a) : (b))

void print_frame() {
    int i, j;
    attron(COLOR_PAIR(1));
    for (j = 0; j < 3; j++ ) {
        for (i = 0; i < COLS; i++) {
            mvaddch(j, i, ' ');
            mvaddch(LINES-1-j, i, ' ');
        }
    }
    for (i = 0; i < LINES-1; i++) {
        mvaddch(i, 0, ' ');
        mvaddch(i, COLS-1, ' ');
    }
    attroff(COLOR_PAIR(1));
}

void print_middle(int y, char *str) {
    int x = max((COLS - strlen(str))/2, 0);
    mvaddstr(y, x, str);
}

void initialize_ncurses() {
    initscr();
    start_color();
    cbreak();
    noecho();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);

    init_pair(1, COLOR_WHITE, COLOR_WHITE);
    init_pair(2, COLOR_YELLOW, COLOR_WHITE);
}

void display_error(const char *str) {
    int x, y;
    int start_x = 2, start_y = 0, width = COLS-4, height = 3;
    int pos;

    x = 0, y = 0, pos = 0;
    attron(COLOR_PAIR(2));
    while (str[pos]) {
        if (x == width) {
            x %= width;
            y++;
        }
        if (y >= height)
            break;
        if (str[pos] == '\n') {
            if (x != 0) {
                y++;
                x = 0;
            }
            pos++;
            continue;
        }
        if (isprint(str[pos]))
            mvaddch(start_y + y, start_x + x, str[pos]);
        x++;
        pos++;
    }
    attroff(COLOR_PAIR(2));
    while (getch() != '\n');

    refresh();
}

void welcome_screen() {
    clear();
    print_middle(7, "Welcome to the chatting application!");
    print_middle(13, "Press ENTER to continue.");
    print_frame();
    refresh();
    while (getch() != '\n');
}

int account_screen() {
    int c, selected, no_options;
    selected = 0;
    no_options = 3;

    c = 0;
    do {
        clear();
        print_middle(4, "What would you like to do?");
        if (c == KEY_UP)
            selected = (selected - 1 + no_options) % no_options;
        else if (c == KEY_DOWN)
            selected = (selected + 1) % no_options;

        if (selected == 0)
            attron(COLOR_PAIR(2));
        print_middle(7, "1) - Create an account.");
        if (selected == 0)
            attroff(COLOR_PAIR(2));

        if (selected == 1)
            attron(COLOR_PAIR(2));
        print_middle(8, "2) - Log into an account.");
        if (selected == 1)
            attroff(COLOR_PAIR(2));

        if (selected == 2)
            attron(COLOR_PAIR(2));
        print_middle(9, "3) - Quit.");
        if (selected == 2)
            attroff(COLOR_PAIR(2));

        print_frame();
        attroff(COLOR_PAIR(1));
        refresh();
    } while ((c = getch()) != '\n');
    return selected+1;
}

void credentials_screen(char *username, char *password) {
    int c;
    size_t username_len, password_len, offset_username, offset_password, max_len;
    c = 0;
    username[0] = '\0';
    password[0] = '\0';
    username_len = 0;
    password_len = 0;
    max_len = COLS - 10;

    do {
        clear();
        print_middle(4, "USER DATA FORM");
        if (c == '\n' && username_len == 0)
            print_middle(5, "Username may not be empty.");

        if (isgraph(c)) {
            username[username_len++] = (char)c;
            username[username_len] = '\0';
        }
        else if(c == KEY_BACKSPACE && username_len > 0) {
            username_len--;
            username[username_len] = '\0';
        }
        offset_username = (username_len > max_len) ? username_len - max_len : 0;

        mvprintw(7, 3, "Enter your username:");
        mvprintw(8, 3, "> %s", username + offset_username);
        mvprintw(9, 3, "Enter your password:");
        mvprintw(10, 3, ">");

        print_frame();
        refresh();
    } while ((c = getch()) != '\n' || username_len == 0);
    c = 0;
    do {
        clear();
        print_middle(4, "USER DATA FORM");
        if (c == '\n' && password_len == 0)
            print_middle(5, "Password may not be empty.");

        if (isgraph(c)) {
            password[password_len++] = (char)c;
            password[password_len] = '\0';
        }
        else if(c == KEY_BACKSPACE && password_len > 0) {
            password_len--;
            password[password_len] = '\0';
        }
        offset_password = (password_len > max_len) ? password_len - max_len : 0;

        mvprintw(7, 3, "Enter your username:");
        mvprintw(8, 3, "> %s", username + offset_username);
        mvprintw(9, 3, "Enter your password:");
        mvprintw(10, 3, "> %s", password + offset_password);

        print_frame();
        refresh();
    } while ((c = getch()) != '\n' || password_len == 0);
}

int loggedin_screen() {
    int c, selected, no_options;
    selected = 0;
    no_options = 5;

    c = 0;
    do {
        clear();
        print_middle(4, "What would you like to do?");
        if (c == KEY_UP)
            selected = (selected - 1 + no_options) % no_options;
        else if (c == KEY_DOWN)
            selected = (selected + 1) % no_options;

        if (selected == 0)
            attron(COLOR_PAIR(2));
        print_middle(7, "1) - Create a chat.");
        if (selected == 0)
            attroff(COLOR_PAIR(2));

        if (selected == 1)
            attron(COLOR_PAIR(2));
        print_middle(8, "2) - Join a chat.");
        if (selected == 1)
            attroff(COLOR_PAIR(2));

        if (selected == 2)
            attron(COLOR_PAIR(2));
        print_middle(9, "3) - List all chats.");
        if (selected == 2)
            attroff(COLOR_PAIR(2));

        if (selected == 3)
            attron(COLOR_PAIR(2));
        print_middle(10, "4) - Log out.");
        if (selected == 3)
            attroff(COLOR_PAIR(2));

        if (selected == 4)
            attron(COLOR_PAIR(2));
        print_middle(11, "5) - Quit.");
        if (selected == 4)
            attroff(COLOR_PAIR(2));

        print_frame();
        refresh();
    } while ((c = getch()) != '\n');
    return selected+1;
}

long join_chat() {
    int c;
    size_t id_len, offset_id, max_len;
    c = 0;
    id_len = 0;
    offset_id = 0;
    max_len = 10;
    char id[12] = {0};

    do {
        clear();
        print_middle(4, "CHAT JOIN");
        if (c == '\n' && id_len == 0)
            print_middle(5, "ID may not be empty.");

        if (isdigit(c) && !(id_len == 0 && c == '0') && id_len <= max_len) {
            id[id_len++] = (char)c;
            id[id_len] = '\0';
        }
        else if(c == KEY_BACKSPACE && id_len > 0) {
            id_len--;
            id[id_len] = '\0';
        }
        mvprintw(7, 3, "Please enter the ID of the chat you would like to join:");
        mvprintw(8, 3, "> %s", id + offset_id);

        attron(COLOR_PAIR(1));
        print_frame();
        attroff(COLOR_PAIR(1));
        refresh();
    } while ((c = getch()) != '\n' || id_len == 0);
    return atol(id);
}

int chats_screen() {
    int c, selected, no_options;
    selected = 0;
    no_options = 4;

    c = 0;
    do {
        clear();
        if (c == KEY_UP)
            selected = (selected - 1 + no_options) % no_options;
        else if (c == KEY_DOWN)
            selected = (selected + 1) % no_options;

        if (selected == 0)
            attron(COLOR_PAIR(2));
        print_middle(7, "1) - Send a message.");
        if (selected == 0)
            attroff(COLOR_PAIR(2));

        if (selected == 1)
            attron(COLOR_PAIR(2));
        print_middle(8, "2) - List all messages.");
        if (selected == 1)
            attroff(COLOR_PAIR(2));

        if (selected == 2)
            attron(COLOR_PAIR(2));
        print_middle(9, "3) - Leave chat.");
        if (selected == 2)
            attroff(COLOR_PAIR(2));

        if (selected == 3)
            attron(COLOR_PAIR(2));
        print_middle(10, "4) - Quit.");
        if (selected == 3)
            attroff(COLOR_PAIR(2));

        print_frame();
        attroff(COLOR_PAIR(1));
        refresh();
    } while ((c = getch()) != '\n');
    return selected+1;
}

void get_message(char *message) {
    int c;
    size_t message_len, offset_message, max_len;
    c = 0;
    message[0] = '\0';
    message_len = 0;
    max_len = COLS - 10;

    do {
        clear();
        print_middle(4, "SEND MESSAGE");
        if (c == '\n' && message_len == 0)
            print_middle(5, "Message may not be empty.");

        if (isprint(c)) {
            message[message_len++] = (char)c;
            message[message_len] = '\0';
        }
        else if(c == KEY_BACKSPACE && message_len > 0) {
            message_len--;
            message[message_len] = '\0';
        }
        offset_message = (message_len > max_len) ? message_len - max_len : 0;

        mvprintw(7, 3, "Enter your message:");
        mvprintw(8, 3, "> %s", message + offset_message);

        print_frame();
        refresh();
    } while ((c = getch()) != '\n' || message_len == 0);
}

void list_messages(const char *str) {
    int c, x, y;
    int start_x = 2, start_y = 4, width = COLS - 4, height = LINES - 8;
    int pos, curr_line = 0, no_lines = 0;

    x = 0, y = 0, pos = 0;
    while (str[pos]) {
        if (x == width) {
            x %= width;
            y++;
        }
        if (str[pos] == '\n') {
            if (x != 0) {
                y++;
                x = 0;
            }
            pos++;
            continue;
        }
        x++;
        pos++;
    }
    no_lines = (x == 0 && y == 0) ? 0 : y;

    c = 0;
    do {
        clear();
        if (c == KEY_UP)
            curr_line = (curr_line > 0) ? curr_line - 1 : 0;
        else if (c == KEY_DOWN) {
            curr_line = (curr_line < no_lines - height) ? curr_line + 1 : curr_line;
        }

        x = 0, y = 0, pos = 0;
        while (str[pos]) {
            if (x == width) {
                x %= width;
                y++;
            }
            if (y >= curr_line)
                break;
            if (str[pos] == '\n') {
                if (x != 0) {
                    y++;
                    x = 0;
                }
                pos++;
                continue;
            }
            x++;
            pos++;
        }

        x = 0, y = 0;
        while (str[pos]) {
            if (x == width) {
                x %= width;
                y++;
            }
            if (y >= height)
                break;
            if (str[pos] == '\n') {
                if (x != 0) {
                    y++;
                    x = 0;
                }
                pos++;
                continue;
            }
            if (isprint(str[pos]))
                mvaddch(start_y + y, start_x + x, str[pos]);
            x++;
            pos++;
        }

        print_frame();
        refresh();
    } while ((c = getch()) != '\n');
}

void goodbye_screen() {
    clear();
    print_middle(7, "Thank you for using this application!");
    print_middle(13, "Press ENTER to continue.");
    print_frame();
    refresh();
    endwin();
}
