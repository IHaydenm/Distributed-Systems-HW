#ifndef SERVER_H
#define SERVER_H

#ifdef _WIN32
    #define _CRT_SECURE_NO_WARNINGS
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>  // Requerido para uint32_t (htonl / ntohl)

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <io.h>

    // Vincula automaticamente la biblioteca de sockets en Visual Studio (MSVC)
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
#endif

#define PORT_NUM 8888

// ---------------------------------------------------------------
// Estructura compleja: una persona con una subestructura (direccion)
// (debe ser identica a la definida en client.h)
// ---------------------------------------------------------------

#define MAX_STR 64      // Tamano maximo de cualquier cadena (incluye '\0')
#define MAX_MSG 512     // Tamano maximo de un mensaje de red

typedef struct {
    char calle[MAX_STR];
    int  numero;
    char ciudad[MAX_STR];
} Direccion;

typedef struct {
    int       id;               // 0 = todavia no almacenada; lo asigna el servidor
    char      nombre[MAX_STR];
    Direccion direccion;        // subestructura
} Persona;

// ---------------------------------------------------------------
// Codigos de retorno de los procedimientos remotos
// ---------------------------------------------------------------

#define RPC_OK             0
#define RPC_NO_ENCONTRADO -1    // el id no existe en el archivo
#define RPC_ERR_ARCHIVO   -2    // no se pudo leer/escribir el archivo
#define RPC_ERR_PROTOCOLO -3    // operacion desconocida o mensaje mal formado

// Identificadores de operacion (viajan al inicio de cada peticion)
#define OP_GUARDAR   1
#define OP_RECUPERAR 2

// Utileria

int initialization(void);
int connection(int socket_desc);
int close_socket(int sock);

// Servicio(s): reciben la estructura POR REFERENCIA (copy-restore)

int guardar_persona(Persona * p);
int recuperar_persona(Persona * p);

#endif
