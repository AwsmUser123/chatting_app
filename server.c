#include <errno.h>
#include <signal.h>
#include <stdbool.h>
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

struct text_data {
    int author_id;
    time_t date;
    char *text;
};

struct chat_data {
    long chat_id;
    struct linked_list **texts;
    int next_member;
    int members[MAX_MEMBERS];
};

struct client_data {
    conn_t *conn;
    int client_id;
    struct chat_data *client_chat;
};

struct linked_list {
    void *element;
    size_t element_len;
    struct linked_list *next;
};

struct arg_data {
    struct client_data *client_data;
    struct linked_list **clients_headptr;
    struct linked_list **chats_headptr;
    pthread_mutex_t *account_list_mutex;
    pthread_mutex_t *client_list_mutex;
    pthread_mutex_t *chat_list_mutex;
};

struct thread_data {
    pthread_t *threads;
    size_t *threads_len;
};

void *safe_malloc               (size_t);
void cleanup                    (void *);
void cleanup_server             (void *);
void join_threads               (void *);
void *accept_connections        (void *);
void *handle_client             (void *);
int login_user                  (struct client_data *, char *, struct linked_list **);
int register_user               (struct client_data *, char *);
struct chat_data *create_chat   (struct client_data *, struct linked_list **);
struct chat_data *join_chat     (struct client_data *, char *, struct linked_list **);
struct chat_data *leave_chat    (struct client_data *, struct linked_list **);
void recv_message               (struct client_data *, char *);
void send_messages              (struct client_data *, char *);
void send_chats                 (struct client_data *, char *, struct linked_list **);
void get_username               (int, char *);
void add_text                   (struct chat_data *, char *, int);
void add_node                   (void *, size_t, struct linked_list **);
bool contains_element           (void *, size_t, struct linked_list **);
void free_element               (void *, struct linked_list **, void (*)(void *));
void free_list                  (struct linked_list **, void (*)(void *));
void free_text_item             (void *);
void free_chat_item             (void *);
void free_client_item           (void *);

int main() {
    pthread_t worker_thread;
    char buf[BUF_SIZE] = {0};

    if (pthread_create(&worker_thread, NULL, accept_connections, NULL) != 0)
        handle_error("Failed to create a thread for the client connection.\n");

    printf("Started the server.\n");
    printf("Type 'quit' to stop this program.\n");

    while (strncmp(buf, "quit", BUF_SIZE))
        if (scanf("%s", buf) < 1)
            handle_error("Failed to read input from user.\n");

    if (pthread_cancel(worker_thread) != 0)
        handle_error("Failed to cancel a thread.\n");
    if (pthread_join(worker_thread, NULL) != 0)
        handle_error("Failed to join a thread.\n");

    printf("Server finished working.\n");
    return 0;
}

void *safe_malloc(size_t size) {
    void *res = malloc(size);
    if (res == NULL)
        handle_error("Failed to allocate memory.\n");
    return res;
}

void cleanup(void *argp) {
    if (argp == NULL)
        return;
    struct arg_data *args = (struct arg_data *)argp;

    //Client connection cleanup
    free_list(args->clients_headptr, free_client_item);
    free_list(args->chats_headptr, free_chat_item);

    pthread_mutex_destroy(args->account_list_mutex);
    pthread_mutex_destroy(args->client_list_mutex);
    pthread_mutex_destroy(args->chat_list_mutex);

    free(args->account_list_mutex);
    free(args->client_list_mutex);
    free(args->chat_list_mutex);
}

void cleanup_server(void *argp) {
    if (argp == NULL)
        return;
    conn_t *args = (conn_t *)argp;

    free_conn(args);
}

void join_threads(void *argp) {
    if (argp == NULL)
        return;
    struct thread_data *args = (struct thread_data *)argp;
    
    for (size_t i = 0; i < *(args->threads_len); i++) {
        if (pthread_cancel(args->threads[i]) != 0)
            handle_error("Failed to cancel a thread.\n");
        if (pthread_join(args->threads[i], NULL) != 0)
            handle_error("Failed to join a thread.\n");
    }
}

void handle_error(const char *msg) {
    fputs(msg, stderr);
    if (errno != 0) {
        fputc(':', stderr);
        fputs(strerror(errno), stderr);
        errno = 0;
    }
    exit(EXIT_FAILURE);
}

void *accept_connections(void *argp) {
    argp = (argp == NULL) ? NULL : argp; //to avoid compiler warnings

    conn_t *server, *client;

    struct client_data *cl_data;
    struct linked_list *chat_list, *client_list;
    struct arg_data args;

    size_t thread_count;
    pthread_t threads[QUEUE_LEN];
    struct thread_data thread_data;

    server = create_server_conn(SERV_ADDR, SERV_PORT);

    pthread_cleanup_push(cleanup, (void *)&args);
    pthread_cleanup_push(cleanup_server, (void *)server);
    pthread_cleanup_push(join_threads, (void *)&thread_data);

    thread_count = 0;
    chat_list = NULL, client_list = NULL;

    thread_data.threads = threads;
    thread_data.threads_len = &thread_count;

    args.clients_headptr = &client_list;
    args.chats_headptr = &chat_list;
    args.account_list_mutex = safe_malloc(sizeof(pthread_mutex_t));
    args.client_list_mutex = safe_malloc(sizeof(pthread_mutex_t));
    args.chat_list_mutex = safe_malloc(sizeof(pthread_mutex_t));

    pthread_mutex_init(args.account_list_mutex, NULL);
    pthread_mutex_init(args.client_list_mutex, NULL);
    pthread_mutex_init(args.chat_list_mutex, NULL);

    while (thread_count < QUEUE_LEN) {
        client = create_accept_conn(server);
        if (client == NULL)
            handle_error("Failed to create a client connection.\n");

        cl_data = safe_malloc(sizeof(struct client_data));

        cl_data->conn = client;
        cl_data->client_id = -1;
        cl_data->client_chat = NULL;

        pthread_mutex_lock(args.client_list_mutex);

        args.client_data = cl_data;
        add_node(args.client_data, sizeof(struct client_data), args.clients_headptr);

        pthread_mutex_unlock(args.client_list_mutex);

        accept_client_conn(client, server);

        if (pthread_create(&threads[thread_count++], NULL, handle_client, (void *)&args) != 0)
            handle_error("Failed to create a thread for the client connection.\n");
        printf("Successfully accepted an incoming connection.\n");
    }

    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);
    return NULL;
} 

void *handle_client(void *argp) {
    struct arg_data *args = (struct arg_data *)argp;
    pthread_mutex_t *account_list_mutex = args->account_list_mutex;
    pthread_mutex_t *client_list_mutex = args->client_list_mutex;
    pthread_mutex_t *chat_list_mutex = args->chat_list_mutex;

    pthread_mutex_lock(client_list_mutex);
    pthread_mutex_lock(chat_list_mutex);

    struct client_data *client_data = args->client_data;
    struct linked_list **clients_list = args->clients_headptr;
    struct linked_list **chats_list = args->chats_headptr;

    pthread_mutex_unlock(client_list_mutex);
    pthread_mutex_unlock(chat_list_mutex);

    char buf[BUF_SIZE];

    while (1) {
        switch (recv_byte(client_data->conn)) {
            case CMD_INIT: {
                send_confirm(client_data->conn);
                break;
            }
            case CMD_REGISTER: {
                if (client_data->client_id != -1)
                    send_error(client_data->conn, "You are already logged in.");
                else {
                    pthread_mutex_lock(account_list_mutex);

                    client_data->client_id = register_user(client_data, buf);

                    pthread_mutex_unlock(account_list_mutex);
                }
                break;
            }
            case CMD_LOGIN: {
                if (client_data->client_id != -1)
                    send_error(client_data->conn, "You are already logged in.");
                else {
                    pthread_mutex_lock(account_list_mutex);
                    pthread_mutex_lock(client_list_mutex);

                    client_data->client_id = login_user(client_data, buf, clients_list);

                    pthread_mutex_unlock(account_list_mutex);
                    pthread_mutex_unlock(client_list_mutex);
                }
                break;
            }
            case CMD_LOGOUT: {
                if (client_data->client_id == -1)
                    send_error(client_data->conn, "You are not logged in.");
                else {
                    client_data->client_id = -1;
                    send_confirm(client_data->conn);
                }
                break;
            }
            case CMD_CREATECHAT: {
                if (client_data->client_id == -1)
                    send_error(client_data->conn, "You are not logged in.");
                else if (client_data->client_chat != NULL)
                    send_error(client_data->conn, "You are already a member of a chat.");
                else {
                    pthread_mutex_lock(chat_list_mutex);

                    client_data->client_chat = create_chat(client_data, chats_list);

                    pthread_mutex_unlock(chat_list_mutex);
                }
                break;
            }
            case CMD_JOINCHAT: {
                if (client_data->client_id == -1)
                    send_error(client_data->conn, "You are not logged in.");
                else if (client_data->client_chat != NULL)
                    send_error(client_data->conn, "You are already a member of a chat.");
                else {
                    pthread_mutex_lock(chat_list_mutex);

                    client_data->client_chat = join_chat(client_data, buf, chats_list);

                    pthread_mutex_unlock(chat_list_mutex);
                }
                break;
            }
            case CMD_LISTCHATS: {
                if (client_data->client_id == -1)
                    send_error(client_data->conn, "You are not logged in.");
                else {
                    pthread_mutex_lock(account_list_mutex);
                    pthread_mutex_lock(chat_list_mutex);

                    send_chats(client_data, buf, chats_list);

                    pthread_mutex_unlock(account_list_mutex);
                    pthread_mutex_unlock(chat_list_mutex);
                }
                break;
            }
            case CMD_QUITCHAT: {
                if (client_data->client_id == -1)
                    send_error(client_data->conn, "You are not logged in.");
                else if (client_data->client_chat == NULL)
                    send_error(client_data->conn, "You are currently not a member of any chat.");
                else {
                    pthread_mutex_lock(chat_list_mutex);

                    client_data->client_chat = leave_chat(client_data, chats_list);

                    pthread_mutex_unlock(chat_list_mutex);
                }
                break;
            }
            case CMD_SENDMSG: {
                if (client_data->client_id == -1)
                    send_error(client_data->conn, "You are not logged in.");
                else if (client_data->client_chat == NULL)
                    send_error(client_data->conn, "You are currently not a member of any chat.");
                else {
                    pthread_mutex_lock(chat_list_mutex);

                    recv_message(client_data, buf);

                    pthread_mutex_unlock(chat_list_mutex);
                }
                break;
            }
            case CMD_LISTMSGS: {
                if (client_data->client_id == -1)
                    send_error(client_data->conn, "You are not logged in.");
                else if (client_data->client_chat == NULL)
                    send_error(client_data->conn, "You are currently not a member of any chat.");
                else {
                    pthread_mutex_lock(account_list_mutex);
                    pthread_mutex_lock(chat_list_mutex);

                    send_messages(client_data, buf);

                    pthread_mutex_unlock(account_list_mutex);
                    pthread_mutex_unlock(chat_list_mutex);
                }
                break;
            }
            case CMD_QUIT: {
                pthread_mutex_lock(client_list_mutex);

                free_element(client_data, clients_list, free_client_item);

                pthread_mutex_unlock(client_list_mutex);
                pthread_exit(NULL);
            }
            default: {
                send_error(client_data->conn, "Server received unknown operation code.");
            }
        }
    }
}

//client_list
int login_user(struct client_data *client_data, char *buf, struct linked_list **head) {
    int index;
    FILE *accounts;
    char *res;
    char account_name[NAME_LEN];
    char account_password[PASS_LEN];

    recv_str(client_data->conn, buf);

    res = strtok(buf, ":\n");
    if (res == NULL) {
        send_error(client_data->conn, "Failed to read account name from client.\n");
        return -1;
    }
    strncpy(account_name, res, NAME_LEN-1);
    res = strtok(NULL, ":\n");
    if (res == NULL) {
        send_error(client_data->conn, "Failed to read account password from client.\n");
        return -1;
    }
    strncpy(account_password, res, PASS_LEN-1);

    if ((accounts = fopen("accounts.txt", "r")) == NULL)
        handle_error("Failed to open accounts file.\n");
    index = -1;
    for (int i = 0; fgets(buf, BUF_SIZE, accounts) != NULL; i++) {
        res = strtok(buf, ":\n");
        if (res == NULL)
            handle_error("Failed to read account name from account file.\n");
        if (strncmp(res, account_name, NAME_LEN) == 0) {
            res = strtok(NULL, ":\n");
            if (res == NULL)
                handle_error("Failed to read account password from account file.\n");
            if (strncmp(res, account_password, PASS_LEN) == 0) {
                struct linked_list *curr = *head;
                while (curr != NULL) {
                    struct client_data *cldata = (struct client_data *)curr->element;
                    if (cldata != NULL && cldata->client_id == i) {
                        if (fclose(accounts) != 0)
                            handle_error("Failed to close accounts file.\n");
                        send_error(client_data->conn, "That user is already logged in.");
                        return -1;
                    }
                    curr = curr->next;
                }
                index = i;
                break;
            }
            else {
                if (fclose(accounts) != 0)
                    handle_error("Failed to close accounts file.\n");
                send_error(client_data->conn, "Wrong password.");
                return -1;
            }
        }
    }
    if (fclose(accounts) != 0)
        handle_error("Failed to close accounts file.\n");
    if (index == -1)
        send_error(client_data->conn, "Account with that username does not exist.");
    else
        send_confirm(client_data->conn);
    return index;
}

int register_user(struct client_data *client_data, char *buf) {
    int index;
    FILE *accounts;
    char *res;
    char account_name[NAME_LEN];
    char account_password[PASS_LEN];

    recv_str(client_data->conn, buf);

    res = strtok(buf, ":\n");
    if (res == NULL) {
        send_error(client_data->conn, "Failed to read account name from client.\n");
        return -1;
    }
    strncpy(account_name, res, NAME_LEN-1);
    res = strtok(NULL, ":\n");
    if (res == NULL) {
        send_error(client_data->conn, "Failed to read account password from client.\n");
        return -1;
    }
    strncpy(account_password, res, PASS_LEN-1);

    if ((accounts = fopen("accounts.txt", "a+")) == NULL)
        handle_error("Failed to open accounts file.\n");
    
    for (index = 0; fgets(buf, BUF_SIZE, accounts) != NULL; index++) {
        res = strtok(buf, ":\n");
        if (res == NULL)
            handle_error("Failed to read account name from account file.\n");
        if (strncmp(res, account_name, NAME_LEN) == 0) {
            if (fclose(accounts) != 0)
                handle_error("Failed to close accounts file.\n");
            send_error(client_data->conn, "Account with that username already exists.");
            return -1;
        }
    }

    if (fprintf(accounts, "%s:%s\n", account_name, account_password) < 0)
        handle_error("Failed to write new data to the accounts file.\n");
    if (fclose(accounts) != 0)
        handle_error("Failed to close accounts file.\n");

    send_confirm(client_data->conn);
    return index;
}

//chat_list
struct chat_data *create_chat(struct client_data *client_data, struct linked_list **head) {
    struct chat_data *tmp;

    tmp = safe_malloc(sizeof(struct chat_data));
    
    tmp->chat_id = random();
    tmp->texts = safe_malloc(sizeof(struct linked_list **));
    *(tmp->texts) = NULL;
    tmp->next_member = 1;
    tmp->members[0] = client_data->client_id;

    add_node(tmp, sizeof(struct chat_data), head);

    send_confirm(client_data->conn);
    return tmp;
}

//chat_list
struct chat_data *join_chat(struct client_data *client_data, char *buf, struct linked_list **head) {
    recv_str(client_data->conn, buf);
    struct chat_data *current_chat;
    struct linked_list *curr = *head;
    long chat_id = atol(buf);
    int found = 0;

    if (curr == NULL)
        return NULL;
    while (curr != NULL) {
        current_chat = (struct chat_data *)curr->element;
        if (current_chat->chat_id == chat_id) {
            found = 1;
            if (current_chat->next_member < MAX_MEMBERS) {
                current_chat->members[current_chat->next_member] = client_data->client_id;
                current_chat->next_member++;
                break;
            }
            else {
                send_error(client_data->conn, "The chat is full.");
                return NULL;
            }
        }
        curr = curr->next;
    }
    if (found == 1) {
        send_confirm(client_data->conn);
        return current_chat;
    }
    else
        send_error(client_data->conn, "Could not find a chat with that ID.");
    return NULL;
}

//chat list used here
struct chat_data *leave_chat(struct client_data *client_data, struct linked_list **head) {
    struct chat_data *curr = client_data->client_chat;
    for (int i = 0; i < curr->next_member; i++) {
        if (curr->members[i] == client_data->client_id) {
            for (int j = i; j < curr->next_member-1; j++)
                curr->members[j] = curr->members[j+1];
            break;
        }
    }
    if (--(curr->next_member) == 0) {
        free_element(curr, head, free_chat_item);
    }
    send_confirm(client_data->conn);
    return NULL;
}

void recv_message(struct client_data *client_data, char *buf) {
    recv_str(client_data->conn, buf);
    add_text(client_data->client_chat, buf, client_data->client_id);
    send_confirm(client_data->conn);
}

void send_messages(struct client_data *client_data, char *buf) {
    struct text_data *texts;
    struct linked_list *curr;
    struct chat_data *chat_data;
    char tmp[BUF_SIZE];
    char username[NAME_LEN];

    strncpy(buf, "", BUF_SIZE);
    chat_data = client_data->client_chat;
    curr = (chat_data->texts == NULL) ? NULL : *(chat_data->texts);

    while (curr != NULL) {
        texts = (struct text_data *)curr->element;
        get_username(texts->author_id, username);
        strncat(buf, username, BUF_SIZE);
        strftime(tmp, BUF_SIZE, " (%b %d %H:%M):\n", localtime(&(texts->date)));
        strncat(buf, tmp, BUF_SIZE);
        strncat(buf, texts->text, BUF_SIZE);
        strncat(buf, "\n", BUF_SIZE);
        curr = curr->next;
    }
    send_str(client_data->conn, buf);
}

//Use chat list here
void send_chats(struct client_data *client_data, char *buf, struct linked_list **head) {
    char tmp[BUF_SIZE];
    struct chat_data *chat_data;
    struct linked_list *curr = *head;

    strncpy(buf, "", BUF_SIZE);
    while (curr != NULL) {
        chat_data = (struct chat_data *)(curr->element);
        if (chat_data == NULL) {
            continue;
        }
        else {
            snprintf(tmp, BUF_SIZE, "Chat ID: %ld\nMembers:\n\t", chat_data->chat_id);
            strncat(buf, tmp, BUF_SIZE);
            for (int i = 0; i < chat_data->next_member; i++) {
                get_username(chat_data->members[i], tmp);
                strncat(buf, tmp, BUF_SIZE);
                strncat(buf, (i == chat_data->next_member - 1) ? "\n" : " ", BUF_SIZE);
            }
        }
        curr = curr->next;
    }
    send_str(client_data->conn, buf);
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

void add_text(struct chat_data *chat_data, char *msg, int author_id) {
    struct text_data *tmp;

    tmp = safe_malloc(sizeof(struct text_data));

    tmp->author_id = author_id;
    tmp->date = time(NULL);
    tmp->text = strndup(msg, BUF_SIZE-1);

    add_node(tmp, sizeof(struct text_data), chat_data->texts);
}

void add_node(void *elem, size_t elem_len, struct linked_list **head) {
    if (head == NULL || elem == NULL || elem_len <= 0)
        return;

    struct linked_list *tmp = NULL, *curr;
    tmp = safe_malloc(sizeof(struct linked_list));
    
    tmp->element = elem;
    tmp->element_len = elem_len;
    tmp->next = NULL;

    curr = *head;
    if (curr == NULL)
        *head = tmp;
    else {
        while (curr->next != NULL)
            curr = curr->next;
        curr->next = tmp;
    }
}

bool contains_element(void *elem, size_t elem_len, struct linked_list **head) {
    if (elem == NULL || elem_len <= 0 || head == NULL)
        return false;
    struct linked_list *curr = *head;
    while (curr != NULL) {
        if (curr->element_len == elem_len && memcmp(elem, curr->element, elem_len) == 0)
            return true;
        curr = curr->next;
    }
    return false;
}

void free_element(void *elem, struct linked_list **head, void (*cleaner_function)(void *elem)) {
    if (head == NULL || elem == NULL)
        return;
    struct linked_list *curr = *head, *prev = NULL;

    while (curr != NULL && curr->element != elem) {
        prev = curr;
        curr = curr->next;
    }
    if (curr == NULL)
        return;

    if (prev == NULL)
        *head = curr->next;
    else
        prev->next = curr->next;
    if (cleaner_function != NULL)
        cleaner_function(curr->element);
    free(curr->element);
    free(curr);
}

void free_list(struct linked_list **head, void (*cleaner_function)(void *elem)) {
    if (head == NULL)
        return;
    struct linked_list *curr = *head, *next;
    while (curr != NULL) {
        next = curr->next;
        if (cleaner_function != NULL)
            cleaner_function(curr->element);
        free(curr->element);
        free(curr);
        curr = next;
    }
    *head = NULL;
}

void free_text_item(void *elem) {
    if (elem == NULL)
        return;
    struct text_data *text_data = (struct text_data *)elem;
    free(text_data->text);
}

void free_chat_item(void *elem) {
    if (elem == NULL)
        return;
    struct chat_data *chat_data = (struct chat_data *)elem;
    free_list(chat_data->texts, free_text_item);
    free(chat_data->texts);
}

void free_client_item(void *elem) {
    if (elem == NULL)
        return;
    struct client_data *client_data = (struct client_data *)elem;
    free_conn(client_data->conn);
}
