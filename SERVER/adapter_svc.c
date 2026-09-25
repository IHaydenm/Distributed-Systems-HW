/* adapter_svc.c
 *
 * Adaptador del servidor (skeleton). Recibe la peticion del cliente,
 * reconstruye la estructura, ejecuta el procedimiento local que corresponde
 * (guardar_persona / recuperar_persona) y regresa el resultado junto con la
 * estructura modificada.
 *
 * Formato de los mensajes (todos los enteros en Network Byte Order):
 *
 *   Peticion : [op][id][nombre][calle][numero][ciudad]
 *   Respuesta: [status][id][nombre][calle][numero][ciudad]
 *
 *   - Enteros (op, id, numero, status): 4 bytes (htonl / ntohl).
 *   - Cadenas: [longitud incluyendo '\0' (4 bytes)][bytes de la cadena].
 *
 * Este archivo contiene el main() del servidor.
 */

#include "server.h"

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

// Garantiza la recepcion de exactamente 'length' bytes.
// Regresa -1 si el cliente cerro la conexion o hubo un error de red.
static int recv_all(int s, char * data, int length)
{
    int total = 0;
    while (total < length) {
        int n = recv(s, data + total, length - total, 0);
        if (n <= 0) return -1;
        total += n;
    }
    return total;
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
// Atencion de una peticion RPC
// ---------------------------------------------------------------

// Regresa 0 si se debe seguir atendiendo al cliente y -1 si hay que
// cerrar la conexion (cliente desconectado o mensaje mal formado).
static int dispatch(int client_sock)
{
    uint32_t op;
    Persona  p;
    Buffer   b;
    int      status;

    // 1. Recibir operacion y copia de la estructura
    if (get_u32(client_sock, &op) < 0)          return -1;
    if (get_persona(client_sock, &p) < 0)       return -1;

    // 2. Ejecutar el procedimiento local (paso por referencia)
    switch (op) {
        case OP_GUARDAR:
            status = guardar_persona(&p);
            break;
        case OP_RECUPERAR:
            status = recuperar_persona(&p);
            break;
        default:
            printf("Unknown operation: %u\n", (unsigned) op);
            status = RPC_ERR_PROTOCOLO;
            break;
    }
    printf("server_result: %d\n", status);

    // 3. Regresar el resultado y la estructura (posiblemente modificada)
    b.len = 0;
    if (put_u32(&b, (uint32_t) status) < 0 || put_persona(&b, &p) < 0) {
        puts("Marshalling failed");
        return -1;
    }
    if (send_all(client_sock, b.data, b.len) < 0) {
        puts("Send failed");
        return -1;
    }
    return 0;
}

int main(void)
{
    int socket_desc;

    setbuf(stdout, NULL);           // bitacora inmediata (aun si se redirige a archivo)

    socket_desc = initialization();
    if (socket_desc < 0) return 1;

    while(1) {
        int client_sock = connection(socket_desc);
        if (client_sock < 0) continue;

        // Bucle para atender multiples peticiones RPC del mismo cliente
        while (dispatch(client_sock) == 0)
            ;

        // Cierre explicito de la conexion del cliente
        close_socket(client_sock);
        puts("Client disconnected. Waiting for new connection...");
    }

    close_socket(socket_desc);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
