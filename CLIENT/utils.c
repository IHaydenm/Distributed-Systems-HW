#include "client.h"
extern char * host;
int connection(void){
    int sock;
    struct sockaddr_in server;

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code : %d\n", WSAGetLastError());
        exit(1);
    }
#endif
    sock = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if(sock == INVALID_SOCKET) {
        printf("Could not create socket. Error Code : %d\n", WSAGetLastError());
        exit(1);
    }
#else
    if(sock == -1) {
        printf("Could not create socket\n");
        exit(1);
    }
#endif
    puts("Socket created");
    memset(&server, 0, sizeof(server));
    server.sin_addr.s_addr = inet_addr(host);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT_NUM);

    if (server.sin_addr.s_addr == INADDR_NONE) {
        printf("Invalid IPv4 address: %s\n", host);
        exit(1);
    }

    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
#ifdef _WIN32
        printf("connect failed. Error: %d\n", WSAGetLastError());
#else
        perror("connect failed. Error");
#endif
        exit(1);
}
    puts("Connected\n");
    return sock;
}

int close_socket(int sock){
    int res;
#ifdef _WIN32
    res = closesocket(sock);
    WSACleanup();
#else
    res = close(sock);
#endif
    return res;
}
