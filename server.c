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

#include "utils.h"

#define SERV_ADDR "192.168.2.186"
#define SERV_PORT 6879
#define BUF_SIZE 1024
#define NAME_LEN 128
#define PASS_LEN 128
#define MAX_MEMBERS 10
#define QUEUE_LEN 5

struct text_list {
    int author;
    time_t date;
    char *text;
    struct text_list *next;
};

struct chat_list {
    long chat_id;
    struct text_list *contents;
    int next_member;
    int members[MAX_MEMBERS];
    struct chat_list *next;
};

struct client_data {
    int client_fd;
    int client_id;
    int chat_id;
};

struct arg_data {
    int clients_len;
    struct client_data client_data[QUEUE_LEN];
    struct chat_list **head;
};

void    cleanup             (int, void *);
void    *accept_connections (void *);
void    *handle_client      (void *);
int     login_user          (struct client_data *, char *);
int     register_user       (struct client_data *, char *);
long    create_chat         (struct client_data *, struct chat_list **);
long    join_chat           (struct client_data *, struct chat_list **);
long    leave_chat          (struct client_data *, struct chat_list **);
void    recv_message        (struct client_data *, char *, struct chat_list **);
void    send_messages       (struct client_data *, char *, struct chat_list **);
void    send_chats          (struct client_data *, char *, struct chat_list **);
void    get_username        (int, char *);
void    add_text            (struct chat_list *, char *, int);
void    free_texts          (struct chat_list *);
void    free_chat           (struct chat_list *, struct chat_list **);

int main() {
    pthread_t worker_thread;

    if (pthread_create(&worker_thread, NULL, accept_connections, NULL) != 0)
        handle_error("Failed to create a thread for the client connection.\n");

    printf("Started the server.\n");
    printf("Press 'q' to stop this program.\n");

    while (getchar() != 'q');

    if (pthread_join(worker_thread, NULL) != 0)
        handle_error("Failed to join a thread.\n");

    printf("Server finished working.\n");
    return 0;
}

void cleanup(int code, void *arg) {
    code = (code == 0) ? 0 : code; //to avoid compiler warnings
    struct chat_list **head = (struct chat_list **)arg;
    struct chat_list *curr = *head;
    while (curr) {
        free_chat(curr, head);
        curr = curr->next;
    }
}

void *accept_connections(void *argp) {
    argp = (argp == NULL) ? NULL : argp; //to avoid compiler warnings
    struct sockaddr_in addr;
    int server, client;
    int i;
    struct sockaddr_in client_addr[QUEUE_LEN];
    socklen_t client_addrlen[QUEUE_LEN];
    pthread_t threads[QUEUE_LEN];
    struct chat_list *first_chat;
    struct arg_data args;

    first_chat = NULL;
    args.clients_len = 0;
    args.head = &first_chat;

    if (on_exit(cleanup, (void *)&first_chat) != 0)
        handle_error("Failed to register a function via on_exit.\n");

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

    for (i = 0; i < QUEUE_LEN; i++) {
        client_addrlen[i] = sizeof(struct sockaddr_in);
        client = accept(server, (struct sockaddr *)(&client_addr[i]), &client_addrlen[i]);
        if (client == -1)
            handle_error("Failed to accept a client connection.\n");
        args.client_data[i].client_fd = client;
        args.client_data[i].client_id = -1;
        args.client_data[i].chat_id = -1;
        args.clients_len++;
        if (pthread_create(&threads[i], NULL, handle_client, (void *)&args) != 0)
            handle_error("Failed to create a thread for the client connection.\n");
        if (pthread_detach(threads[i]) != 0)
            handle_error("Failed to detach a thread.\n");
        printf("Successfully accepted an incoming connection.\n");
    }
    return NULL;
} 

void *handle_client(void *argp) {
    struct arg_data *args = (struct arg_data *)argp;
    int idx = args->clients_len-1;
    struct client_data client_data = args->client_data[idx];
    struct chat_list **head = args->head;
    char buf[BUF_SIZE];

    while (1) {
        switch (recv_byte(client_data.client_fd)) {
            case 0x01: {
                send_confirm(client_data.client_fd);
                break;
            }
            case 0x03: {
                if (client_data.client_id != -1)
                    send_error(client_data.client_fd, "You are already logged in.");
                else
                    client_data.client_id = register_user(&client_data, buf);
                break;
            }
            case 0x05: {
                if (client_data.client_id != -1)
                    send_error(client_data.client_fd, "You are already logged in.");
                else
                    client_data.client_id = login_user(&client_data, buf);
                break;
            }
            case 0x06: {
                if (client_data.client_id == -1)
                    send_error(client_data.client_fd, "You are not logged in.");
                else {
                    client_data.client_id = -1;
                    send_confirm(client_data.client_fd);
                }
                break;
            }
            case 0x07: {
                if (client_data.client_id == -1)
                    send_error(client_data.client_fd, "You are not logged in.");
                else if (client_data.chat_id != -1)
                    send_error(client_data.client_fd, "You are already a member of a chat.");
                else
                    client_data.chat_id = create_chat(&client_data, head);
                break;
            }
            case 0x09: {
                if (client_data.client_id == -1)
                    send_error(client_data.client_fd, "You are not logged in.");
                else if (client_data.chat_id != -1)
                    send_error(client_data.client_fd, "You are already a member of a chat.");
                else
                    client_data.chat_id = join_chat(&client_data, head);
                break;
            }
            case 0x0a: {
                if (client_data.client_id == -1)
                    send_error(client_data.client_fd, "You are not logged in.");
                else if (client_data.chat_id == -1)
                    send_error(client_data.client_fd, "You are currently not a member of any chat.");
                else
                    client_data.chat_id = leave_chat(&client_data, head);
                break;
            }
            case 0x0b: {
                if (client_data.client_id == -1)
                    send_error(client_data.client_fd, "You are not logged in.");
                else if (client_data.chat_id == -1)
                    send_error(client_data.client_fd, "You are currently not a member of any chat.");
                else
                    recv_message(&client_data, buf, head);
                break;
            }
            case 0x0d: {
                if (client_data.client_id == -1)
                    send_error(client_data.client_fd, "You are not logged in.");
                else if (client_data.chat_id == -1)
                    send_error(client_data.client_fd, "You are currently not a member of any chat.");
                else
                    send_messages(&client_data, buf, head);
                break;
            }
            case 0x10: {
                if (client_data.client_id == -1)
                    send_error(client_data.client_fd, "You are not logged in.");
                else
                    send_chats(&client_data, buf, head);
                break;
            }
            case 0x20: {
                pthread_exit(NULL);
            }
            default: {
                send_error(client_data.client_fd, "Server received unknown operation code.");
            }
        }
    }
}

int login_user(struct client_data *client_data, char *buf) {
    int index;
    FILE *accounts;
    char *res;
    char account_name[NAME_LEN];
    char account_password[PASS_LEN];

    recv_str(client_data->client_fd, buf);

    res = strtok(buf, ":\n");
    if (res == NULL) {
        send_error(client_data->client_fd, "Failed to read account name from client.\n");
        return -1;
    }
    strcpy(account_name, res);
    res = strtok(NULL, ":\n");
    if (res == NULL) {
        send_error(client_data->client_fd, "Failed to read account password from client.\n");
        return -1;
    }
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
                send_error(client_data->client_fd, "Wrong password.");
                return -1;
            }
        }
    }
    if (fclose(accounts) != 0)
        handle_error("Failed to close accounts file.\n");
    if (index == -1)
        send_error(client_data->client_fd, "Account with that username does not exist.");
    else
        send_confirm(client_data->client_fd);
    return index;
}

int register_user(struct client_data *client_data, char *buf) {
    int index;
    FILE *accounts;
    char *res;
    char account_name[NAME_LEN];
    char account_password[PASS_LEN];

    recv_str(client_data->client_fd, buf);

    res = strtok(buf, ":\n");
    if (res == NULL) {
        send_error(client_data->client_fd, "Failed to read account name from client.\n");
        return -1;
    }
    strcpy(account_name, res);
    res = strtok(NULL, ":\n");
    if (res == NULL) {
        send_error(client_data->client_fd, "Failed to read account password from client.\n");
        return -1;
    }
    strcpy(account_password, res);

    if ((accounts = fopen("accounts.txt", "a+")) == NULL)
        handle_error("Failed to open accounts file.\n");
    
    for (index = 0; fgets(buf, BUF_SIZE, accounts) != NULL; index++) {
        res = strtok(buf, ":\n");
        if (res == NULL)
            handle_error("Failed to read account name from account file.\n");
        if (!strcmp(res, account_name)) {
            send_error(client_data->client_fd, "Account with that username already exists.");
            return -1;
        }
    }
    if (fprintf(accounts, "%s:%s\n", account_name, account_password) < 0)
        handle_error("Failed to write new data to the accounts file.\n");
    if (fclose(accounts) != 0)
        handle_error("Failed to close accounts file.\n");
    send_confirm(client_data->client_fd);
    return index;
}

long create_chat(struct client_data *client_data, struct chat_list **head) {
    struct chat_list *curr;
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
    curr->next_member = 1;
    curr->members[0] = client_data->client_id;
    curr->next = NULL;

    send_confirm(client_data->client_fd);
    return curr->chat_id;
}

long join_chat(struct client_data *client_data, struct chat_list **head) {
    long chat_id;
    int found = 0;
    if (read(client_data->client_fd, &chat_id, 8) < 8)
        handle_error("Failed to read data from client.\n");
    if (*head != NULL) {
        struct chat_list *curr = *head;
        while (curr != NULL) {
            if (curr->chat_id == chat_id) {
                found = 1;
                if (curr->next_member < MAX_MEMBERS) {
                    curr->members[curr->next_member] = client_data->client_id;
                    curr->next_member++;
                    break;
                }
                else {
                    send_error(client_data->client_fd, "The chat is full.");
                    return -1;
                }
            }
            curr = curr->next;
        }
    }
    if (found == 1) {
        send_confirm(client_data->client_fd);
        return chat_id;
    }
    else {
        send_error(client_data->client_fd, "Could not find a chat with that ID.");
        return -1;
    }
}

long leave_chat(struct client_data *client_data, struct chat_list **head) {
    struct chat_list *curr = *head;
    while (curr && curr->chat_id != client_data->chat_id)
        curr = curr->next;
    if (curr == NULL)
        handle_error("Could not find a chat with that ID.\n");
    for (int i = 0; i < curr->next_member; i++) {
        if (curr->members[i] == client_data->client_id) {
            for (int j = i; j < curr->next_member-1; j++)
                curr->members[j] = curr->members[j+1];
            break;
        }
    }
    if (--(curr->next_member) == 0) {
        free_chat(curr, head);
    }
    send_confirm(client_data->client_fd);
    return -1;
}

void recv_message(struct client_data *client_data, char *buf, struct chat_list **head) {
    struct chat_list *curr = *head;

    while (curr && curr->chat_id != client_data->chat_id)
        curr = curr->next;
    if (curr == NULL)
        handle_error("Could not find a chat with that ID.\n");

    recv_str(client_data->client_fd, buf);
    add_text(curr, buf, client_data->client_id);
    send_confirm(client_data->client_fd);
}

void send_messages(struct client_data *client_data, char *buf, struct chat_list **head) {
    struct chat_list *curr = *head;
    struct text_list *texts;
    char tmp[BUF_SIZE];
    char username[NAME_LEN];

    while (curr && curr->chat_id != client_data->chat_id)
        curr = curr->next;
    if (curr == NULL)
        handle_error("Could not find a chat with that ID.\n");

    texts = curr->contents;
    strcpy(buf, "");
    while (texts != NULL) {
        get_username(texts->author, username);
        strcat(buf, username);
        strftime(tmp, BUF_SIZE, " (%b %d %H:%M):\n", localtime(&(texts->date)));
        strcat(buf, tmp);
        strcat(buf, texts->text);
        strcat(buf, "\n");
        texts = texts->next;
    }
    send_str(client_data->client_fd, buf);
}

void send_chats(struct client_data *client_data, char *buf, struct chat_list **head) {
    char tmp[BUF_SIZE];
    struct chat_list *curr = *head;
    strcpy(buf, "");
    while (curr != NULL) {
        sprintf(tmp, "Chat ID: %ld\n", curr->chat_id);
        strcat(buf, tmp);
        strcat(buf, "Members:\n\t");
        for (int i = 0; i < curr->next_member; i++) {
            get_username(curr->members[i], tmp);
            strcat(buf, tmp);
            strcat(buf, (i == curr->next_member - 1) ? "\n" : " ");
        }
        curr = curr->next;
    }
    send_str(client_data->client_fd, buf);
}

void get_username(int client_id, char *buf) {
    char *res;
    FILE *accounts;
    int i = 0;
    if ((accounts = fopen("accounts.txt", "r")) == NULL)
        handle_error("Failed to open accounts file.\n");
    while (fgets(buf, BUF_SIZE, accounts) != NULL && i < client_id)
        i++;
    if (i < client_id)
        handle_error("An account with the given ID does not exist.\n");
    res = strtok(buf, ":\n");
    if (res == NULL)
        handle_error("Failed to read account name from account file.\n");
    if (fclose(accounts) != 0)
        handle_error("Failed to close accounts file.\n");
}

void add_text(struct chat_list *item, char *msg, int author_id) {
    struct text_list *curr, *tmp;
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
    tmp->author = author_id;
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

void free_chat(struct chat_list *item, struct chat_list **head) {
    struct chat_list *prev = NULL, *curr = *head;
    while (curr && curr != item) {
        prev = curr;
        curr = curr->next;
    }
    if (prev == NULL)
        *head = curr->next;
    else
        prev->next = curr->next;
    free_texts(curr);
    free(curr);
}

void handle_error(const char *msg) {
    char buf[BUF_SIZE];
    strcpy(buf, msg);
    if (errno != 0) {
        strcat(buf, "\n");
        strcat(buf, strerror(errno));
        errno = 0;
    }
    fputs(buf, stderr);
    exit(EXIT_FAILURE);
}
