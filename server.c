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
    char *text;
    time_t *date;
    struct text_list *next;
};

struct chat_item {
    long chat_id;
    struct text_list *contents;
    int curr_member;
    int members[MAX_MEMBERS];
    struct chat_item *next;
};

struct arg_data {
    int client_fd;
    struct chat_item **head;
};

void send_message(int, char *);
void send_confirmation(int);
void send_error(int, char *);
void handle_error(char *);
void *handle_client(void *);
int login_user(int);
void register_user(int);
void create_chat(int, int, struct chat_item **);
void join_chat(int, int, struct chat_item **);
void get_username(int, char *);

int main() {
    struct sockaddr_in addr;
    int server, client;
    int i;
    struct sockaddr_in clients[QUEUE_LEN];
    socklen_t client_len[QUEUE_LEN];
    pthread_t threads[QUEUE_LEN];
    struct chat_item *first_chat;

    i = 0;
    first_chat = NULL;
    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1)
        handle_error("Socket creation failed.\n");

    addr.sin_family = AF_INET;
    addr.sin_port = SERV_PORT;
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
    char buf[BUF_SIZE];
    int data_len;
    data_len = strlen(msg)+1;
    memcpy(buf, &data_len, 4);
    strcpy(buf+4, msg);
    if (write(client_fd, buf, data_len+4) < data_len+4)
        handle_error("Failed to send the data to the client.\n");
}

void send_error(int client_fd, char *msg) {
    char buf[BUF_SIZE];
    int data_len;
    buf[0] = 0x00;
    if (errno != 0)
        sprintf(buf+5, "%s\nError:%s", msg, strerror(errno));
    else
        sprintf(buf+5, "%s", msg);
    data_len = strlen(buf+5)+1;
    memcpy(buf+1, &data_len, 4);
    if (write(client_fd, buf, data_len+5) < data_len+5)
        handle_error("Failed to send the error to the client.\n");
}

void *handle_client(void *arg) {
    int client = ((struct arg_data *)arg)->client_fd;
    struct chat_item **head = ((struct arg_data *)arg)->head;
    int client_id = -1;
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
                    send_error(client, "You are already logged in.\n");
                else
                    client_id = login_user(client);
                break;
            }
            case 0x07: {
                create_chat(client, client_id, head);
                break;
            }
            case 0x09: {
                join_chat(client, client_id, head);
                break;
            }
            case 0x10: {
                char tmp[BUF_SIZE];
                char username[NAME_LEN];
                struct chat_item *curr = *head;
                strcpy(buf, "");
                while (curr != NULL) {
                    sprintf(tmp, "CHAT ID: %ld\n", curr->chat_id);
                    strcat(buf, tmp);
                    strcat(buf, "Members:\n");
                    for (int j = 0; j < curr->curr_member; j++) {
                        get_username(curr->members[j], username);
                        sprintf(tmp, "\t%s\n", username);
                        strcat(buf, tmp);
                    }
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
                send_error(client_fd, "Wrong password.\n");
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
            send_error(client_fd, "Account with that username already exists.\n");
            return;
        }
    }
    if (fprintf(accounts, "%s:%s\n", account_name, account_password) < 0)
        handle_error("Failed to write new data to the accounts file.\n");
    send_confirmation(client_fd);
}

void create_chat(int client_fd, int client_id, struct chat_item **head) {
    struct chat_item *curr;
    char buf[BUF_SIZE];
    if (client_id == -1) {
        send_error(client_fd, "You are not logged in.\n");
        return;
    }
    if (*head == NULL) {
        *head = malloc(sizeof(struct chat_item));
        if (*head == NULL)
            handle_error("Failed to allocate memory.\n");
        curr = *head;
    }
    else {
        curr = *head;
        while (curr->next != NULL)
            curr = curr->next;
        curr->next = malloc(sizeof(struct chat_item));
        if (curr->next == NULL)
            handle_error("Failed to allocate memory.\n");
        curr = curr->next;
    }
    curr->chat_id = random();
    curr->contents = NULL;
    curr->curr_member = 1;
    curr->members[0] = client_id;
    curr->next = NULL;
    sprintf(buf, "CHAT ID: %ld\n", curr->chat_id);
    send_message(client_fd, buf);
    send_confirmation(client_fd);
}

void join_chat(int client_fd, int client_id, struct chat_item **head) {
    if (client_id == -1) {
        send_error(client_fd, "You are not logged in.\n");
        return;
    }
    long chat_id;
    int found = 0;
    if (read(client_fd, &chat_id, 4) < 4)
        handle_error("Failed to read data from client.\n");
    if (*head != NULL) {
        struct chat_item *curr = *head;
        while (curr->next != NULL) {
            if (curr->chat_id == chat_id) {
                found = 1;
                if (curr->curr_member < MAX_MEMBERS) {
                    for (int i = 0; i < curr->curr_member; i++) {
                        if (curr->members[i] == client_id) {
                            send_error(client_fd, "You are already a member of that chat.\n");
                            return;
                        }
                    }
                    curr->members[curr->curr_member] = client_id;
                    curr->curr_member++;
                    break;
                }
                else {
                    send_error(client_fd, "The chat is full.\n");
                    return;
                }
            }
            curr = curr->next;
        }
    }
    if (found == 1)
        send_confirmation(client_fd);
    else 
        send_error(client_fd, "Could not find a chat with that ID.\n");
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
