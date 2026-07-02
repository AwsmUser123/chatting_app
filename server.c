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

void handle_error(char *);
void *handle_client(void *);

int main() {
    struct sockaddr_in addr;
    int server, client;
    int i;
    struct sockaddr_in clients[QUEUE_LEN];
    socklen_t client_len[QUEUE_LEN];
    pthread_t threads[QUEUE_LEN];

    i = 0;
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
        if (pthread_create(&threads[i], NULL, handle_client, (void *)&client) != 0)
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

void *handle_client(void *arg) {
    int client = *((int *)arg);
    int index;
    int client_id = -1;
    char buf[BUF_SIZE];

    while (1) {
        if (read(client, buf, 1) < 1)
            handle_error("Failed to receive data from client.\n");
        switch (buf[0]) {
            case 0x00: {
                //Error
                pthread_exit(NULL);
            }
            case 0x01: {
                buf[0] = 0x02;
                if (write(client, buf, 1) < 1) {
                    handle_error("Failed to send data to client.\n");
                }
                break;
            }
            case 0x05: {
                int data_len;
                FILE *accounts;
                char *res;
                char account_name[NAME_LEN];
                char account_password[PASS_LEN];

                if (client_id != -1)
                    handle_error("The client is already logged in.\n");

                if (read(client, &data_len, 4) < 4)
                    handle_error("Failed to read data size.\n");
                if (data_len >= BUF_SIZE)
                    handle_error("Data provided by client is too big.\n");
                if (read(client, buf, data_len) < data_len)
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
                index = 0;
                while (fgets(buf, BUF_SIZE, accounts) != NULL) {
                    res = strtok(buf, ":\n");
                    if (res == NULL)
                        handle_error("Failed to read account name from account file.\n");
                    if (!strcmp(res, account_name)) {
                        res = strtok(NULL, ":\n");
                        if (res == NULL)
                            handle_error("Failed to read account password from account file.\n");
                        if (!strcmp(res, account_password)) {
                            client_id = index;
                            buf[0] = 0x06;
                            if (write(client, buf, 1) < 1)
                                handle_error("Failed to send login confirmation to client.\n");
                            break;
                        }
                        else
                            handle_error("Wrong password.\n");
                    }
                }
                if (client_id == -1)
                    handle_error("Account with that username does not exist.\n");
                break;
            }
            case 0x20: {
                //Regular quit
                pthread_exit(NULL);
            }
            default: {
                handle_error("Server received unknown operation code.\n");
            }
        }
    }
}
