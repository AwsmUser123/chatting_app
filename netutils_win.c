#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include "constants.h"
#include "netutils.h"

WSADATA wsaData;

struct connection {
    SOCKET socket;
    bool use_tls;
};

void init_net() {
    int res;
    res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0)
        printf("WSAStartup failed: %d\n", res);
}

void end_net() {
    WSACleanup();
}

conn_t *create_client_conn(const char *ip_string, const char *port) {
    conn_t *connection = malloc(sizeof(conn_t));
    if (connection == NULL) {
        printf("Failed to allocate memory.\n");
        return NULL;
    }
    connection->use_tls = false;

    struct addrinfo *result = NULL, *ptr = NULL, hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int res = getaddrinfo(ip_string, port, &hints, &result);
    if (res != 0) {
        printf("getaddrinfo failed: %d\n", res);
        free(connection);
        WSACleanup();
        return NULL;
    }

    SOCKET connect_socket;
    for (ptr=result; ptr != NULL; ptr = ptr->ai_next) {

        connect_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connect_socket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            free(connection);
            WSACleanup();
            return NULL;
        }

        res = connect(connect_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (res == SOCKET_ERROR) {
            closesocket(connect_socket);
            connect_socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (connect_socket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        free(connection);
        WSACleanup();
        return NULL;
    }

    connection->socket = connect_socket;

    return connection;
}

conn_t *create_server_conn(const char *ip_string, const char *port) {
    conn_t *connection = malloc(sizeof(conn_t));
    if (connection == NULL) {
        printf("Failed to allocate memory.\n");
        return NULL;
    }
    connection->use_tls = false;
    int res;

    SOCKET listen_socket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    res = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (res != 0) {
        printf("getaddrinfo failed with error: %d\n", res);
        free(connection);
        WSACleanup();
        return NULL;
    }

    listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listen_socket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        free(connection);
        WSACleanup();
        return NULL;
    }

    res = bind( listen_socket, result->ai_addr, (int)result->ai_addrlen);
    if (res == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(listen_socket);
        free(connection);
        WSACleanup();
        return NULL;
    }

    freeaddrinfo(result);

    res = listen(listen_socket, SOMAXCONN);
    if (res == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        free(connection);
        WSACleanup();
        return NULL;
    }

    connection->socket = listen_socket;
    return connection;
}

conn_t *create_accept_conn() {
    conn_t *connection = malloc(sizeof(conn_t));
    if (connection == NULL) {
        printf("Failed to allocate memory.\n");
        return NULL;
    }
    connection->use_tls = false;
    connection->socket = INVALID_SOCKET;

    return connection;
}

void accept_client_conn(conn_t *client, conn_t *server) {
    SOCKET client_socket = INVALID_SOCKET;
    client_socket = accept(server->socket, NULL, NULL);
    if (client_socket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(server->socket);
        WSACleanup();
        return;
    }
    client->socket = client_socket;
}

void close_conn(conn_t *connection) {
    if (connection == NULL)
        return;
    closesocket(connection->socket);
    connection->socket = INVALID_SOCKET;
    WSACleanup();
}

conn_t *free_conn(conn_t *connection) {
    free(connection);
}

ssize_t read_conn(conn_t *connection, void *buf, size_t count) {
    if (connection == NULL)
        return -1;
    char *p = buf;
    while (count > 0) {
        ssize_t received = recv(connection->socket, p, count, 0);
        if (received <= 0)
            return -1;
        p += received;
        count -= received;
    }
    return 0;
}

ssize_t write_conn(conn_t *connection, const void *buf, size_t count) {
    if (connection == NULL)
        return -1;
    const char *p = buf;
    while (count > 0) {
        ssize_t written = send(connection->socket, p, count, 0);
        if (written <= 0)
            return -1;
        p += written;
        count -= written;
    }
    return 0;
}
