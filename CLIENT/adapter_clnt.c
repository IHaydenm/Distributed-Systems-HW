/* adapter_clnt.c
 *
 * Adaptador del cliente (stub). Convierte una llamada local a
 * guardar_persona() / recuperar_persona() en un intercambio de mensajes
 * por el socket.
 *
 * Formato de los mensajes (todos los enteros en Network Byte Order):
 *
 *   Peticion : [op][id][nombre][calle][numero][ciudad]
 *   Respuesta: [status][id][nombre][calle][numero][ciudad]
 *
 *   - Enteros (op, id, numero, status): 4 bytes (htonl / ntohl).
 *   - Cadenas: [longitud incluyendo '\0' (4 bytes)][bytes de la cadena].
 *
 * La estructura se envia CAMPO POR CAMPO y no como un bloque de memoria,
 * porque sizeof(Persona), el relleno (padding) y el orden de bytes pueden
 * variar entre compiladores y arquitecturas.
 *
 * Paso por REFERENCIA con semantica copy-restore:
 *   1. copy    : se envia una copia de *p al servidor.
 *   2. el servidor la modifica y la regresa.
 *   3. restore : la copia recibida sobrescribe el contenido de *p.
 */

#include "client.h"

extern int sock;

typedef struct {
    char data[MAX_MSG];
    int  len;
} Buffer;

// ---------------------------------------------------------------
// Entrada/salida completa sobre el socket
// ---------------------------------------------------------------

// Garantiza el envio completo de los datos
static int send_all(int s, const char * data, int length)
{
    int total_sent = 0;
    while (total_sent < length) {
        int sent = send(s, data + total_sent, length - total_sent, 0);
        if (sent <= 0) return -1;
        total_sent += sent;
    }
    return total_sent;
}

// Garantiza la recepcion de exactamente 'length' bytes
static int recv_all(int s, char * data, int length)
{
    int total = 0;
    while (total < length) {
        int n = recv(s, data + total, length - total, 0);
        if (n == 0) {
            puts("Server disconnected unexpectedly");
            return -1;
        }
        if (n < 0) {
#ifdef _WIN32
            printf("recv failed. Error: %d\n", WSAGetLastError());
#else
            perror("recv failed");
#endif
            return -1;
        }
        total += n;
    }
    return total;
}

// ---------------------------------------------------------------
// Empaquetado (marshalling): estructura -> buffer
// ---------------------------------------------------------------

static int put_u32(Buffer * b, uint32_t value)
{
    uint32_t net = htonl(value);                    // Host -> Network
    if (b->len + (int) sizeof(net) > MAX_MSG) return -1;
    memcpy(b->data + b->len, &net, sizeof(net));
    b->len += (int) sizeof(net);
    return 0;
}

// 's' debe apuntar a un arreglo de MAX_STR caracteres
static int put_str(Buffer * b, const char * s)
{
    const char * end = (const char *) memchr(s, '\0', MAX_STR);
    uint32_t length;

    if (end == NULL) return -1;                     // cadena sin terminador
    length = (uint32_t)(end - s) + 1;               // incluye el '\0'

    if (put_u32(b, length) < 0) return -1;
    if (b->len + (int) length > MAX_MSG) return -1;
    memcpy(b->data + b->len, s, length);
    b->len += (int) length;
    return 0;
}

static int put_persona(Buffer * b, const Persona * p)
{
    if (put_u32(b, (uint32_t) p->id) < 0)               return -1;
    if (put_str(b, p->nombre) < 0)                      return -1;
    if (put_str(b, p->direccion.calle) < 0)             return -1;
    if (put_u32(b, (uint32_t) p->direccion.numero) < 0) return -1;
    if (put_str(b, p->direccion.ciudad) < 0)            return -1;
    return 0;
}

// ---------------------------------------------------------------
// Desempaquetado (unmarshalling): socket -> estructura
// ---------------------------------------------------------------

static int get_u32(int s, uint32_t * value)
{
    uint32_t net;
    if (recv_all(s, (char *) &net, sizeof(net)) < 0) return -1;
    *value = ntohl(net);                            // Network -> Host
    return 0;
}

// 'dst' debe apuntar a un arreglo de MAX_STR caracteres
static int get_str(int s, char * dst)
{
    uint32_t length;

    if (get_u32(s, &length) < 0) return -1;
    if (length == 0 || length > MAX_STR) {
        puts("Protocol error: invalid string length");
        return -1;
    }
    if (recv_all(s, dst, (int) length) < 0) return -1;
    dst[length - 1] = '\0';                         // terminador garantizado
    return 0;
}

static int get_persona(int s, Persona * p)
{
    uint32_t id, numero;

    memset(p, 0, sizeof(Persona));

    if (get_u32(s, &id) < 0)                        return -1;
    p->id = (int) id;
    if (get_str(s, p->nombre) < 0)                  return -1;
    if (get_str(s, p->direccion.calle) < 0)         return -1;
    if (get_u32(s, &numero) < 0)                    return -1;
    p->direccion.numero = (int) numero;
    if (get_str(s, p->direccion.ciudad) < 0)        return -1;
    return 0;
}

// ---------------------------------------------------------------
// Llamada remota generica
// ---------------------------------------------------------------

static int llamar(uint32_t op, Persona * p)
{
    Buffer   b;
    Persona  recibida;
    uint32_t status;

    b.len = 0;

    // 1. Empaquetar operacion + copia de la estructura
    if (put_u32(&b, op) < 0 || put_persona(&b, p) < 0) {
        puts("Marshalling failed");
        return RPC_ERR_PROTOCOLO;
    }

    // 2. Enviar la peticion completa
    if (send_all(sock, b.data, b.len) < 0) {
        puts("Send request failed");
        return RPC_ERR_PROTOCOLO;
    }

    // 3. Recibir el resultado y la estructura modificada
    if (get_u32(sock, &status) < 0 || get_persona(sock, &recibida) < 0)
        return RPC_ERR_PROTOCOLO;

    // 4. "Restore": la copia del servidor reemplaza la estructura del cliente
    *p = recibida;

    return (int) status;
}

// ---------------------------------------------------------------
// Procedimientos remotos (interfaz identica a la de una llamada local)
// ---------------------------------------------------------------

int guardar_persona(Persona * p)
{
    return llamar(OP_GUARDAR, p);
}

int recuperar_persona(Persona * p)
{
    return llamar(OP_RECUPERAR, p);
}
