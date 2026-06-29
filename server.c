#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define SERV_ADDR "192.168.2.186"
#define SERV_PORT 6879
#define QUEUE_LEN 5

int main() {
    struct sockaddr_in addr;
    int fd, client;
    int running;
    int i;
    char buf;
    struct sockaddr_in clients[QUEUE_LEN];

    buf = 0;
    running = 1;
    i = 0;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = SERV_PORT;
    inet_aton(SERV_ADDR, &(addr.sin_addr));
    bind(fd, (struct sockaddr *)(&addr), sizeof(addr));
    listen(fd, QUEUE_LEN);
    

    do {
        client = accept(fd, (struct sockaddr *)(clients + i), sizeof(clients[i]));
        i++;
        spawn_thread(client);
    } while (running);
    return 0;
} 
