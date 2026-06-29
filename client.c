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
    int server;
    char buf;

    buf = 0;
    server = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = SERV_PORT;
    inet_aton(SERV_ADDR, &(addr.sin_addr));

    connect(server, (struct sockaddr *)(&addr), sizeof(addr));
    while (1) {
        read(0, &buf, 1);
        write(server, &buf, 1);
    }
    return 0;
} 
