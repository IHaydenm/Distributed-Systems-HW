#ifndef SERVER_H
#define SERVER_H

#ifdef _WIN32
    #define _CRT_SECURE_NO_WARNINGS
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <io.h>

    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
#endif

#define PORT_NUM 8888

#define MAX_STR 64
#define MAX_MSG 512

typedef struct {
    char street[MAX_STR];
    int  number;
    char city[MAX_STR];
} Address;

typedef struct {
    int     id;
    char    name[MAX_STR];
    Address address;
} Person;

#define RPC_OK             0
#define RPC_NOT_FOUND      -1
#define RPC_FILE_ERROR     -2
#define RPC_PROTOCOL_ERROR -3

#define OP_SAVE     1
#define OP_RETRIEVE 2

int initialization(void);
int connection(int socket_desc);
int close_socket(int sock);

int save_person(Person * p);
int retrieve_person(Person * p);

#endif
