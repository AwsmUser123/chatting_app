#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "codes.h"
#include "constants.h"
#include "utils.h"


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
    struct sockaddr_in client_addr;
    socklen_t client_addrlen;
};

struct arg_data {
    int *clients_len;
    struct client_data client_data[QUEUE_LEN];
    struct chat_list **head;
    pthread_mutex_t *account_list_mutex;
    pthread_mutex_t *client_list_mutex;
    pthread_mutex_t *chat_list_mutex;
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
    char buf[BUF_SIZE] = {0};

    if (pthread_create(&worker_thread, NULL, accept_connections, NULL) != 0)
        handle_error("Failed to create a thread for the client connection.\n");

    printf("Started the server.\n");
    printf("Type 'quit' to stop this program.\n");

    while (strcmp(buf, "quit"))
        if (scanf("%s", buf) < 1)
            handle_error("Failed to read input from user.\n");

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
    int i, thread_count;
    struct sockaddr_in tmp_addr;
    socklen_t tmp_addrlen;
    pthread_t threads[QUEUE_LEN];
    struct chat_list *first_chat;
    struct arg_data args;
    pthread_mutex_t mutex_accounts, mutex_clients, mutex_chats;

    first_chat = NULL;
    i = 0;
    thread_count = 0;
    args.clients_len = &i;
    args.head = &first_chat;
    args.account_list_mutex = &mutex_accounts;
    args.client_list_mutex = &mutex_clients;
    args.chat_list_mutex = &mutex_chats;
    pthread_mutex_init(args.account_list_mutex, NULL);
    pthread_mutex_init(args.client_list_mutex, NULL);
    pthread_mutex_init(args.chat_list_mutex, NULL);

    on_exit(cleanup, (void *)&first_chat);

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

    while (i < QUEUE_LEN && thread_count < QUEUE_LEN) {
        tmp_addrlen = sizeof(struct sockaddr_in);
        client = accept(server, (struct sockaddr *)(&tmp_addr), &tmp_addrlen);
        if (client == -1)
            handle_error("Failed to accept a client connection.\n");

        pthread_mutex_lock(args.client_list_mutex);
        args.client_data[i].client_addr = tmp_addr;
        args.client_data[i].client_addrlen = tmp_addrlen;
        args.client_data[i].client_fd = client;
        args.client_data[i].client_id = -1;
        args.client_data[i].chat_id = -1;
        *(args.clients_len) += 1;
        pthread_mutex_unlock(args.client_list_mutex);

        if (pthread_create(&threads[thread_count++], NULL, handle_client, (void *)&args) != 0)
            handle_error("Failed to create a thread for the client connection.\n");
        printf("Successfully accepted an incoming connection.\n");
    }

    for (int j = 0; j < thread_count; j++)
        if (pthread_join(threads[j], NULL) != 0)
            handle_error("Failed to join a thread.\n");
    pthread_mutex_destroy(args.account_list_mutex);
    pthread_mutex_destroy(args.client_list_mutex);
    pthread_mutex_destroy(args.chat_list_mutex);
    return NULL;
} 

void *handle_client(void *argp) {
    struct arg_data *args = (struct arg_data *)argp;
    pthread_mutex_t *account_list_mutex = args->account_list_mutex;
    pthread_mutex_t *client_list_mutex = args->client_list_mutex;
    pthread_mutex_t *chat_list_mutex = args->chat_list_mutex;

    pthread_mutex_lock(client_list_mutex);
    pthread_mutex_lock(chat_list_mutex);

    int idx = *(args->clients_len) - 1;
    struct client_data client_data = args->client_data[idx];
    struct chat_list **head = args->head;

    pthread_mutex_unlock(client_list_mutex);
    pthread_mutex_unlock(chat_list_mutex);

    char buf[BUF_SIZE];

    while (1) {
        switch (recv_byte(client_data.client_fd)) {
            case CMD_INIT: {
                send_confirm(client_data.client_fd);
                break;
            }
            case CMD_REGISTER: {
                if (client_data.client_id != -1)
                    send_error(client_data.client_fd, "You are already logged in.");
                else {
                    pthread_mutex_lock(account_list_mutex);
                    client_data.client_id = register_user(&client_data, buf);
                    pthread_mutex_unlock(account_list_mutex);
                }
                break;
            }
            case CMD_LOGIN: {
                if (client_data.client_id != -1)
                    send_error(client_data.client_fd, "You are already logged in.");
                else {
                    pthread_mutex_lock(account_list_mutex);
                    client_data.client_id = login_user(&client_data, buf);
                    pthread_mutex_unlock(account_list_mutex);
                }
                break;
            }
            case CMD_LOGOUT: {
                if (client_data.client_id == -1)
                    send_error(client_data.client_fd, "You are not logged in.");
                else {
                    client_data.client_id = -1;
                    send_confirm(client_data.client_fd);
                }
                break;
            }
            case CMD_CREATECHAT: {
                if (client_data.client_id == -1)
                    send_error(client_data.client_fd, "You are not logged in.");
                else if (client_data.chat_id != -1)
                    send_error(client_data.client_fd, "You are already a member of a chat.");
                else {
                    pthread_mutex_lock(chat_list_mutex);
                    client_data.chat_id = create_chat(&client_data, head);
                    pthread_mutex_unlock(chat_list_mutex);
                }
                break;
            }
            case CMD_JOINCHAT: {
                if (client_data.client_id == -1)
                    send_error(client_data.client_fd, "You are not logged in.");
                else if (client_data.chat_id != -1)
                    send_error(client_data.client_fd, "You are already a member of a chat.");
                else {
                    pthread_mutex_lock(chat_list_mutex);
                    client_data.chat_id = join_chat(&client_data, head);
                    pthread_mutex_unlock(chat_list_mutex);
                }
                break;
            }
            case CMD_LISTCHATS: {
                if (client_data.client_id == -1)
                    send_error(client_data.client_fd, "You are not logged in.");
                else {
                    pthread_mutex_lock(account_list_mutex);
                    pthread_mutex_lock(chat_list_mutex);
                    send_chats(&client_data, buf, head);
                    pthread_mutex_unlock(account_list_mutex);
                    pthread_mutex_unlock(chat_list_mutex);
                }
                break;
            }
            case CMD_QUITCHAT: {
                if (client_data.client_id == -1)
                    send_error(client_data.client_fd, "You are not logged in.");
                else if (client_data.chat_id == -1)
                    send_error(client_data.client_fd, "You are currently not a member of any chat.");
                else {
                    pthread_mutex_lock(chat_list_mutex);
                    client_data.chat_id = leave_chat(&client_data, head);
                    pthread_mutex_unlock(chat_list_mutex);
                }
                break;
            }
            case CMD_SENDMSG: {
                if (client_data.client_id == -1)
                    send_error(client_data.client_fd, "You are not logged in.");
                else if (client_data.chat_id == -1)
                    send_error(client_data.client_fd, "You are currently not a member of any chat.");
                else {
                    pthread_mutex_lock(chat_list_mutex);
                    recv_message(&client_data, buf, head);
                    pthread_mutex_unlock(chat_list_mutex);
                }
                break;
            }
            case CMD_LISTMSGS: {
                if (client_data.client_id == -1)
                    send_error(client_data.client_fd, "You are not logged in.");
                else if (client_data.chat_id == -1)
                    send_error(client_data.client_fd, "You are currently not a member of any chat.");
                else {
                    pthread_mutex_lock(account_list_mutex);
                    pthread_mutex_lock(chat_list_mutex);
                    send_messages(&client_data, buf, head);
                    pthread_mutex_unlock(account_list_mutex);
                    pthread_mutex_unlock(chat_list_mutex);
                }
                break;
            }
            case CMD_QUIT: {
                pthread_mutex_lock(client_list_mutex);
                idx = *(args->clients_len);
                int account_idx = -1;
                for (int i = 0; i < idx; i++) {
                    if (args->client_data[i].client_fd == client_data.client_fd) {
                        account_idx = i;
                        break;
                    }
                }
                if (account_idx == -1)
                    handle_error("Could not find current client in client list.\n");
                for (int i = account_idx; i < idx-1; i++)
                    args->client_data[i] = args->client_data[i+1];
                *(args->clients_len) -= 1;
                pthread_mutex_unlock(client_list_mutex);
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
    long chat_id = recv_u64(client_data->client_fd);
    int found = 0;
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
    tmp->author = author_id;
    tmp->date = time(NULL);
    tmp->text = strdup(msg);
    tmp->next = NULL;

    curr = item->contents;
    if (curr == NULL)
        item->contents = tmp;
    else {
        while (curr->next != NULL)
            curr = curr->next;
        curr->next = tmp;
    }
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
        strcat(buf, ":");
        strcat(buf, strerror(errno));
        errno = 0;
    }
    fputs(buf, stderr);
    exit(EXIT_FAILURE);
}
