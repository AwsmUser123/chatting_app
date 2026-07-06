#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERV_ADDR "192.168.2.186"
#define SERV_PORT 6879
#define QUEUE_LEN 5
#define BUF_SIZE 1024
#define NAME_LEN 128
#define PASS_LEN 128
#define MAX_MEMBERS 10

struct text_list {
    int author;
    time_t date;
    char *text;
    struct text_list *next;
};

struct chat_list {
    long chat_id;
    struct text_list *contents;
    int curr_member;
    int members[MAX_MEMBERS];
    struct chat_list *next;
};

struct arg_data {
    int client_fd;
    struct chat_list **head;
};

void send_message(int, char *);
void receive_message(int, char *);
void send_confirmation(int);
void send_error(int, char *);
void handle_error(char *);
void *handle_client(void *);
int login_user(int);
void register_user(int);
int create_chat(int, int, struct chat_list **);
int join_chat(int, int, struct chat_list **);
void get_username(int, char *);
void add_text(struct chat_list *, char *, int);
void free_texts(struct chat_list *);

int main() {
    struct sockaddr_in addr;
    int server, client;
    int i;
    struct sockaddr_in clients[QUEUE_LEN];
    socklen_t client_len[QUEUE_LEN];
    pthread_t threads[QUEUE_LEN];
    struct chat_list *first_chat;

    i = 0;
    first_chat = NULL;
    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1)
        handle_error("Socket creation failed.\n");

    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERV_PORT);
    if (inet_aton(SERV_ADDR, &(addr.sin_addr)) == 0)
        handle_error("Invalid server address.\n");
    if (bind(server, (struct sockaddr *)(&addr), sizeof(addr)) == -1)
        handle_error("Failed to bind the address to the socket.\n");
    if (listen(server, QUEUE_LEN) == -1)
        handle_error("Failed to mark the socket as listening.\n");
    
    printf("Successfully started the server.\n");
    while (i < QUEUE_LEN) {
        client_len[i] = sizeof(struct sockaddr_in);
        client = accept(server, (struct sockaddr *)(&clients[i]), &client_len[i]);
        if (client == -1)
            handle_error("Failed to accept a client connection.\n");
        struct arg_data args;
        args.client_fd = client;
        args.head = &first_chat;
        if (pthread_create(&threads[i], NULL, handle_client, (void *)&args) != 0)
            handle_error("Failed to create a thread for the client connection.\n");
        printf("Successfully accepted an incoming connection.\n");
        i++;
    }

    for (i = 0; i < QUEUE_LEN; i++) {
        if (pthread_join(threads[i], NULL) != 0)
            handle_error("Failed to join a thread.\n");
    }

    printf("Server finished working.\n");
    return 0;
} 

void handle_error(char *msg) {
    if (errno != 0)
        perror(msg);
    else
        fprintf(stderr, "%s", msg);
    exit(EXIT_FAILURE);
}

void send_confirmation(int client_fd) {
    char code = 0x02;
    if (write(client_fd, &code, 1) < 1)
        handle_error("Failed to send confirmation to client.\n");
} 

void send_message(int client_fd, char *msg) {
    int data_len;
    data_len = strlen(msg)+1;
    if (write(client_fd, &data_len, 4) < 4)
        handle_error("Failed to send the data to the client.\n");
    if (write(client_fd, msg, data_len) < data_len)
        handle_error("Failed to send the data to the client.\n");
}

void receive_message(int client_fd, char *dst) {
    int data_len;
    if (read(client_fd, &data_len, 4) < 4)
        handle_error("Failed to read the data from the server.\n");
    if (read(client_fd, dst, data_len) < data_len)
        handle_error("Failed to read the data from the server.\n");
}

void send_error(int client_fd, char *msg) {
    char buf[BUF_SIZE];
    int data_len;
    buf[0] = 0x00;
    if (errno != 0)
        sprintf(buf+5, "%s:%s", msg, strerror(errno));
    else
        sprintf(buf+5, "%s", msg);
    data_len = strlen(buf+5)+1;
    memcpy(buf+1, &data_len, 4);
    if (write(client_fd, buf, data_len+5) < data_len+5)
        handle_error("Failed to send the error to the client.\n");
}

void *handle_client(void *arg) {
    int client = ((struct arg_data *)arg)->client_fd;
    struct chat_list **head = ((struct arg_data *)arg)->head;
    int client_id = -1;
    long current_chat = -1;
    char buf[BUF_SIZE];

    while (1) {
        if (read(client, buf, 1) < 1)
            handle_error("Failed to receive data from client.\n");
        switch (buf[0]) {
            case 0x01: {
                send_confirmation(client);
                break;
            }
            case 0x03: {
                register_user(client);
                break;
            }
            case 0x05: {
                if (client_id != -1)
                    send_error(client, "You are already logged in.");
                else
                    client_id = login_user(client);
                break;
            }
            case 0x06: {
                if (client_id == -1)
                    send_error(client, "You are not logged in.");
                else {
                    client_id = -1;
                    send_confirmation(client);
                }
                break;
            }
            case 0x07: {
                if (current_chat != -1)
                    send_error(client, "You are already a member of a chat.");
                else
                    current_chat = create_chat(client, client_id, head);
                break;
            }
            case 0x09: {
                if (current_chat != -1)
                    send_error(client, "You are already a member of a chat.");
                else
                    current_chat = join_chat(client, client_id, head);
                break;
            }
            case 0x0a: {
                if (current_chat == -1) {
                    send_error(client, "You are currently not a member of any chat.");
                    break;
                }
                struct chat_list *prev = NULL, *curr = *head;
                while (curr && curr->chat_id != current_chat) {
                    prev = curr;
                    curr = curr->next;
                }
                if (curr == NULL)
                    handle_error("Could not find a chat with that ID.\n");
                for (int i = 0; i < curr->curr_member; i++) {
                    if (curr->members[i] == client_id) {
                        for (int j = i; j < curr->curr_member-1; j++)
                            curr->members[j] = curr->members[j+1];
                        break;
                    }
                }
                if (--(curr->curr_member) == 0) {
                    if (prev == NULL)
                        *head = curr->next;
                    else
                        prev->next = curr->next;
                    free_texts(curr);
                    free(curr);
                }
                current_chat = -1;
                send_confirmation(client);
                break;
            }
            case 0x0b: {
                if (current_chat == -1) {
                    send_error(client, "You are currently not a member of any chat.");
                    break;
                }
                struct chat_list *curr = *head;
                while (curr->chat_id != current_chat && curr)
                    curr = curr->next;
                if (curr == NULL)
                    handle_error("Could not find a chat with that ID.\n");
                receive_message(client, buf);
                add_text(curr, buf, client_id);
                send_confirmation(client);
                break;
            }
            case 0x0d: {
                if (current_chat == -1) {
                    send_error(client, "You are currently not a member of any chat.");
                    break;
                }
                struct chat_list *curr = *head;
                struct text_list *texts;
                char tmp[BUF_SIZE];
                char username[NAME_LEN];

                while (curr->chat_id != current_chat && curr)
                    curr = curr->next;
                if (curr == NULL)
                    handle_error("Could not find a chat with that ID.\n");

                texts = curr->contents;
                strcpy(buf, "");
                while (texts != NULL) {
                    get_username(texts->author, username);
                    strcat(buf, username);
                    strftime(tmp, BUF_SIZE, " (%b %d %H:%M)", localtime(&(texts->date));
                    strcat(buf, tmp);
                    strcat(buf, texts->text);
                    strcat(buf, "\n");
                    texts = texts->next;
                }
                send_message(client, buf);
                break;
            }
            case 0x10: {
                char tmp[BUF_SIZE];
                char username[NAME_LEN];
                struct chat_list *curr = *head;
                strcpy(buf, "");
                while (curr != NULL) {
                    sprintf(tmp, "Chat ID: %ld\n", curr->chat_id);
                    strcat(buf, tmp);
                    strcat(buf, "Members: %ld\n", curr->curr_member);
                    curr = curr->next;
                }
                send_message(client, buf);
                break;
            }
            case 0x20: {
                pthread_exit(NULL);
            }
            default: {
                handle_error("Server received unknown operation code.\n");
            }
        }
    }
}

int login_user(int client_fd) {
    int data_len;
    int index;
    FILE *accounts;
    char *res;
    char buf[BUF_SIZE];
    char account_name[NAME_LEN];
    char account_password[PASS_LEN];

    if (read(client_fd, &data_len, 4) < 4)
        handle_error("Failed to read data size.\n");
    if (data_len >= BUF_SIZE)
        handle_error("Data provided by client is too big.\n");
    if (read(client_fd, buf, data_len) < data_len)
        handle_error("Failed to receive login data from client.\n");
    buf[data_len] = '\0';

    res = strtok(buf, ":\n");
    if (res == NULL)
        handle_error("Failed to read account name from client.\n");
    strcpy(account_name, res);
    res = strtok(NULL, ":\n");
    if (res == NULL)
        handle_error("Failed to read account password from client.\n");
    strcpy(account_password, res);

    if ((accounts = fopen("accounts.txt", "r")) == NULL)
        handle_error("Failed to open accounts file.\n");
    index = -1;
    for (int i = 0; fgets(buf, BUF_SIZE, accounts) != NULL; i++) {
        res = strtok(buf, ":\n");
        if (res == NULL)
            handle_error("Failed to read account name from account file.\n");
        if (!strcmp(res, account_name)) {
            res = strtok(NULL, ":\n");
            if (res == NULL)
                handle_error("Failed to read account password from account file.\n");
            if (!strcmp(res, account_password)) {
                index = i;
                break;
            }
            else {
                send_error(client_fd, "Wrong password.");
                return -1;
            }
        }
    }
    if (index == -1)
        handle_error("Account with that username does not exist.\n");
    else
        send_confirmation(client_fd);
    return index;
}

void register_user(int client_fd) {
    int data_len;
    FILE *accounts;
    char *res;
    char buf[BUF_SIZE];
    char account_name[NAME_LEN];
    char account_password[PASS_LEN];

    if (read(client_fd, &data_len, 4) < 4)
        handle_error("Failed to read data size.\n");
    if (data_len >= BUF_SIZE)
        handle_error("Data provided by client is too big.\n");
    if (read(client_fd, buf, data_len) < data_len)
        handle_error("Failed to receive login data from client.\n");
    buf[data_len] = '\0';

    res = strtok(buf, ":\n");
    if (res == NULL)
        handle_error("Failed to read account name from client.\n");
    strcpy(account_name, res);
    res = strtok(NULL, ":\n");
    if (res == NULL)
        handle_error("Failed to read account password from client.\n");
    strcpy(account_password, res);

    if ((accounts = fopen("accounts.txt", "r+")) == NULL)
        handle_error("Failed to open accounts file.\n");
    while (fgets(buf, BUF_SIZE, accounts) != NULL) {
        res = strtok(buf, ":\n");
        if (res == NULL)
            handle_error("Failed to read account name from account file.\n");
        if (!strcmp(res, account_name)) {
            send_error(client_fd, "Account with that username already exists.");
            return;
        }
    }
    if (fprintf(accounts, "%s:%s\n", account_name, account_password) < 0)
        handle_error("Failed to write new data to the accounts file.\n");
    send_confirmation(client_fd);
}

int create_chat(int client_fd, int client_id, struct chat_list **head) {
    struct chat_list *curr;
    char buf[BUF_SIZE];
    if (client_id == -1) {
        send_error(client_fd, "You are not logged in.");
        return -1;
    }
    if (*head == NULL) {
        *head = malloc(sizeof(struct chat_list));
        if (*head == NULL)
            handle_error("Failed to allocate memory.\n");
        curr = *head;
    }
    else {
        curr = *head;
        while (curr->next != NULL)
            curr = curr->next;
        curr->next = malloc(sizeof(struct chat_list));
        if (curr->next == NULL)
            handle_error("Failed to allocate memory.\n");
        curr = curr->next;
    }
    curr->chat_id = random();
    curr->contents = NULL;
    curr->curr_member = 1;
    curr->members[0] = client_id;
    curr->next = NULL;

    send_confirmation(client_fd);
    return curr->chat_id;
}

int join_chat(int client_fd, int client_id, struct chat_list **head) {
    if (client_id == -1) {
        send_error(client_fd, "You are not logged in.");
        return -1;
    }
    long chat_id;
    int found = 0;
    if (read(client_fd, &chat_id, 8) < 8)
        handle_error("Failed to read data from client.\n");
    if (*head != NULL) {
        struct chat_list *curr = *head;
        while (curr != NULL) {
            if (curr->chat_id == chat_id) {
                found = 1;
                if (curr->curr_member < MAX_MEMBERS) {
                    curr->members[curr->curr_member] = client_id;
                    curr->curr_member++;
                    break;
                }
                else {
                    send_error(client_fd, "The chat is full.");
                    return -1;
                }
            }
            curr = curr->next;
        }
    }
    if (found == 1) {
        send_confirmation(client_fd);
        return chat_id;
    }
    else {
        send_error(client_fd, "Could not find a chat with that ID.");
        return -1;
    }
}

void get_username(int client_id, char *result) {
    char buf[BUF_SIZE];
    char *res;
    FILE *accounts;
    int i = 0;
    if ((accounts = fopen("accounts.txt", "r")) == NULL)
        handle_error("Failed to open accounts file.\n");
    while (fgets(buf, BUF_SIZE, accounts) != NULL) {
        res = strtok(buf, ":\n");
        if (res == NULL)
            handle_error("Failed to read account name from account file.\n");
        if (i == client_id) {
            strcpy(result, res);
            return;
        }
        i++;
    }
}

void add_text(struct chat_list *item, char *msg, int author) {
    struct text_list *curr, *tmp;
    if (item == NULL)
        handle_error("Empty list provided.\n");
    if (msg == NULL)
        handle_error("Empty string provided.\n");
    if ((tmp = malloc(sizeof(struct text_list))) == NULL)
        handle_error("Failed to allocate memory.\n");
    curr = item->contents;
    if (curr == NULL)
        item->contents = tmp;
    else {
        while (curr->next != NULL)
            curr = curr->next;
        curr->next = tmp;
    }
    tmp->author = author;
    tmp->date = time(NULL);
    tmp->text = strdup(msg);
    tmp->next = NULL;
}

void free_texts(struct chat_list *item) {
    if (item == NULL)
        return;
    struct text_list *curr = item->contents, *next;
    while (curr != NULL) {
        next = curr->next;
        free(curr->text);
        free(curr);
        curr = next;
    }
}
