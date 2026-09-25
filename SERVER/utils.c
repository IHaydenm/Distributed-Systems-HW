#include "server.h"
#ifndef _WIN32
    #include <signal.h>
#endif

int initialization(void){
    int socket_desc;
    struct sockaddr_in server;

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code : %d\n", WSAGetLastError());
        return -1;
    }
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

#ifdef _WIN32
    if(socket_desc == INVALID_SOCKET) {
        printf("Could not create socket. Error Code : %d\n", WSAGetLastError());
        return -1;
    }
#else
    if(socket_desc == -1) {
        printf("Could not create socket\n");
        return -1;
    }

    {
        int opt = 1;
        setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }
#endif
    puts("Socket created");
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( PORT_NUM );

    if(bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
#ifdef _WIN32
        printf("bind failed. Error: %d\n", WSAGetLastError());
#else
        perror("bind failed. Error");
#endif
        return -1;
    }
    puts("bind done");
    listen(socket_desc, 3);
    puts("Waiting for incoming connections...");
    return socket_desc;
}

int connection(int socket_desc){
    struct sockaddr_in client;
    int client_sock;
    socklen_t c = sizeof(client);

    client_sock = accept( socket_desc,
                          (struct sockaddr *) &client,
                          &c );

    if(client_sock < 0) {
#ifdef _WIN32
        printf("accept failed. Error: %d\n", WSAGetLastError());
#else
        perror("accept failed");
#endif
        return -1;
    }
    puts("Connection accepted");

    return client_sock;
}

int close_socket(int sock){
#ifdef _WIN32
    return closesocket(sock);
#else
    return close(sock);
#endif
}
