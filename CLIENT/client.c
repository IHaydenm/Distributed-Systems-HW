/* client.c
 *
 * Inicializa y modifica una estructura Persona y usa los servicios remotos
 * guardar_persona() y recuperar_persona() como si fueran procedimientos
 * locales. Lo unico agregado para la distribucion es la conexion con el
 * servidor (connection / close_socket).
 */

#include "client.h"

int sock;
char * host;

// Copia una cadena sin desbordar el arreglo destino
static void copiar_cadena(char * dst, const char * src)
{
    strncpy(dst, src, MAX_STR - 1);
    dst[MAX_STR - 1] = '\0';
}

// Inicializa la estructura completa (incluida la subestructura)
static void inicializar_persona(Persona * p, const char * nombre,
                                const char * calle, int numero,
                                const char * ciudad)
{
    memset(p, 0, sizeof(Persona));
    p->id = 0;                                  // aun no existe en el servidor
    copiar_cadena(p->nombre, nombre);
    copiar_cadena(p->direccion.calle, calle);
    p->direccion.numero = numero;
    copiar_cadena(p->direccion.ciudad, ciudad);
}

static void imprimir_persona(const char * titulo, const Persona * p)
{
    printf("%s\n", titulo);
    printf("    id        : %d\n", p->id);
    printf("    nombre    : %s\n", p->nombre);
    printf("    direccion : %s #%d, %s\n\n",
           p->direccion.calle, p->direccion.numero, p->direccion.ciudad);
}

// Secuencia de llamadas de la practica
static int demo(Persona * p)
{
    Persona q;
    int r;

    // 1. La estructura solo existe en el cliente
    imprimir_persona("[1] Estructura inicializada en el cliente:", p);

    // 2. Alta: el servidor recibe una copia, le asigna un id, la guarda en
    //    su archivo y la regresa; la copia recibida reemplaza a *p
    r = guardar_persona(p);                     // llamada local (servicio)
    printf("    guardar_persona() -> %d\n\n", r);
    if (r != RPC_OK) return r;
    imprimir_persona("[2] Estructura despues de la llamada (id asignado por el servidor):", p);

    // 3. Modificacion: se cambia la subestructura y se actualiza el registro
    copiar_cadena(p->direccion.calle, "Calle 5 de Mayo");
    p->direccion.numero = 45;
    copiar_cadena(p->direccion.ciudad, "Cholula");
    imprimir_persona("[3] Estructura modificada en el cliente:", p);

    r = guardar_persona(p);
    printf("    guardar_persona() -> %d (registro actualizado)\n\n", r);
    if (r != RPC_OK) return r;

    // 4. Recuperacion: se envia una estructura vacia con solo el id y el
    //    servidor la regresa completa, leida de su archivo
    memset(&q, 0, sizeof(Persona));
    q.id = p->id;
    r = recuperar_persona(&q);
    printf("    recuperar_persona(id=%d) -> %d\n\n", p->id, r);
    if (r != RPC_OK) return r;
    imprimir_persona("[4] Estructura recuperada del archivo del servidor:", &q);

    // 5. Un id que no existe: el servidor reporta el error y la estructura
    //    del cliente queda intacta
    memset(&q, 0, sizeof(Persona));
    q.id = 9999;
    r = recuperar_persona(&q);
    printf("[5] recuperar_persona(id=9999) -> %d (%s)\n", r,
           r == RPC_NO_ENCONTRADO ? "no encontrado, como se esperaba"
                                  : "resultado inesperado");

    return (r == RPC_NO_ENCONTRADO) ? RPC_OK : RPC_ERR_PROTOCOLO;
}

int main( int argc, char * argv[] )
{
    Persona p;
    int result;

    if( argc != 2 && argc != 6 ) {
        fprintf(stderr, "Uso: %s <host> [<nombre> <calle> <numero> <ciudad>]\n",
                argv[0]);
        exit( 1 );
    }

    host = argv[1];

    if (argc == 6)
        inicializar_persona(&p, argv[2], argv[3], atoi(argv[4]), argv[5]);
    else
        inicializar_persona(&p, "Juan Perez", "Av. Reforma", 123, "Puebla");

    sock = connection();            // Conexion con el servidor
                                    // para llamar servicios

    result = demo(&p);

    close_socket(sock);             // Cerrar la conexion y limpiar red
    return (result == RPC_OK) ? 0 : 1;
}
