#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERV_ADDR "192.168.2.186"
#define SERV_PORT 6879
#define QUEUE_LEN 5

void handle_error(char *);
void *handle_client(void *);

int main() {
    struct sockaddr_in addr;
    int server, client;
    int i;
    char buf;
    struct sockaddr_in clients[QUEUE_LEN];
    socklen_t client_len[QUEUE_LEN];
    pthread_t threads[QUEUE_LEN];

    buf = 0x00;
    i = 0;
    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1) {
        handle_error("Socket creation failed.\n");
    }

    addr.sin_family = AF_INET;
    addr.sin_port = SERV_PORT;
    if (inet_aton(SERV_ADDR, &(addr.sin_addr)) == 0) {
        handle_error("Invalid server address.\n");
    }
    if (bind(server, (struct sockaddr *)(&addr), sizeof(addr)) == -1) {
        handle_error("Failed to bind the address to the socket.\n");
    }
    if (listen(server, QUEUE_LEN) == -1) {
        handle_error("Failed to mark the socket as listening.\n");
    }
    
    while (i < QUEUE_LEN) {
        client_len[i] = sizeof(struct sockaddr_in);
        client = accept(server, (struct sockaddr *)(&clients[i]), &client_len[i]);
        if (client == -1) {
            handle_error("Failed to accept a client connection.\n");
        }
        if (pthread_create(&threads[i], NULL, handle_client, (void *)&client) != 0) {
            handle_error("Failed to create a thread for the client connection.\n");
        }
        i++;
    }

    for (i = 0; i < QUEUE_LEN; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            handle_error("Failed to join a thread.\n");
        }
    }

    printf("Server finished working.\n");
    return 0;
} 

void *handle_client(void *arg) {
    int client = *((int *)arg);
    char buf;
    buf = 0x00;

    while (1) {
        if (read(client, &buf, 1) < 1) {
            handle_error("Failed to receive data from client.\n");
        }
        if (buf == 0x01) {
            buf = 0x02;
            if (write(client, &buf, 1) < 1) {
                handle_error("Failed to send data to client.\n");
            }
            pthread_exit(NULL);
        }
    }
}

void handle_error(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}
