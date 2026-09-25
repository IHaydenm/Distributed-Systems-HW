#include "server.h"

#ifndef _WIN32
    #include <signal.h>
#endif

int initialization(void)
{
    int socket_desc;
    struct sockaddr_in server;

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code : %d\n", WSAGetLastError());
        return -1;
    }
#else
    // Si un cliente se cae a media respuesta, send() debe regresar un error
    // en lugar de terminar el proceso del servidor con SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif

    // Create socket
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

    // Permite reiniciar el servidor sin esperar a que el puerto se libere
    {
        int opt = 1;
        setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }
#endif
    puts("Socket created");

    // Prepare the sockaddr_in structure
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( PORT_NUM );

    // Bind
    if(bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
#ifdef _WIN32
        printf("bind failed. Error: %d\n", WSAGetLastError());
#else
        perror("bind failed. Error");
#endif
        return -1;
    }
    puts("bind done");

    // Listen (Se debe invocar una sola vez al inicializar el servicio)
    listen(socket_desc, 3);
    puts("Waiting for incoming connections...");

    return socket_desc;
}

int connection(int socket_desc)
{
    struct sockaddr_in client;
    int client_sock;
    socklen_t c = sizeof(client);

    // Accept connection from an incoming client
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

// Implementacion de close_socket para abstraer POSIX vs Winsock
int close_socket(int sock)
{
#ifdef _WIN32
    return closesocket(sock);
#else
    return close(sock);
#endif
}
