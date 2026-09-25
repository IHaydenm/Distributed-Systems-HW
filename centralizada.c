/* centralizada.c
 *
 * Version CENTRALIZADA de la aplicacion: cliente y servicios en un solo
 * programa. Las funciones guardar_persona() y recuperar_persona() son
 * exactamente las de SERVER/server.c, y la secuencia de llamadas es la de
 * CLIENT/client.c; aqui se invocan de forma local, sin sockets.
 *
 * Compilar: cc centralizada.c -o centralizada
 * Ejecutar: ./centralizada [<nombre> <calle> <numero> <ciudad>]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STR 64

typedef struct {
    char calle[MAX_STR];
    int  numero;
    char ciudad[MAX_STR];
} Direccion;

typedef struct {
    int       id;
    char      nombre[MAX_STR];
    Direccion direccion;
} Persona;

#define RPC_OK             0
#define RPC_NO_ENCONTRADO -1
#define RPC_ERR_ARCHIVO   -2

#define ARCHIVO "personas.dat"

int guardar_persona(Persona * p);
int recuperar_persona(Persona * p);

// ---------------------------------------------------------------
// Cliente
// ---------------------------------------------------------------

static void copiar_cadena(char * dst, const char * src)
{
    strncpy(dst, src, MAX_STR - 1);
    dst[MAX_STR - 1] = '\0';
}

static void inicializar_persona(Persona * p, const char * nombre,
                                const char * calle, int numero,
                                const char * ciudad)
{
    memset(p, 0, sizeof(Persona));
    p->id = 0;
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

int main( int argc, char * argv[] )
{
    Persona p, q;
    int r;

    if( argc != 1 && argc != 5 ) {
        fprintf(stderr, "Uso: %s [<nombre> <calle> <numero> <ciudad>]\n", argv[0]);
        exit( 1 );
    }

    if (argc == 5)
        inicializar_persona(&p, argv[1], argv[2], atoi(argv[3]), argv[4]);
    else
        inicializar_persona(&p, "Juan Perez", "Av. Reforma", 123, "Puebla");

    imprimir_persona("[1] Estructura inicializada:", &p);

    r = guardar_persona(&p);                    // llamada local
    printf("    guardar_persona() -> %d\n\n", r);
    if (r != RPC_OK) return 1;
    imprimir_persona("[2] Estructura despues de la llamada (id asignado):", &p);

    copiar_cadena(p.direccion.calle, "Calle 5 de Mayo");
    p.direccion.numero = 45;
    copiar_cadena(p.direccion.ciudad, "Cholula");
    imprimir_persona("[3] Estructura modificada:", &p);

    r = guardar_persona(&p);
    printf("    guardar_persona() -> %d (registro actualizado)\n\n", r);
    if (r != RPC_OK) return 1;

    memset(&q, 0, sizeof(Persona));
    q.id = p.id;
    r = recuperar_persona(&q);
    printf("    recuperar_persona(id=%d) -> %d\n\n", p.id, r);
    if (r != RPC_OK) return 1;
    imprimir_persona("[4] Estructura recuperada del archivo:", &q);

    memset(&q, 0, sizeof(Persona));
    q.id = 9999;
    r = recuperar_persona(&q);
    printf("[5] recuperar_persona(id=9999) -> %d (%s)\n", r,
           r == RPC_NO_ENCONTRADO ? "no encontrado, como se esperaba"
                                  : "resultado inesperado");

    return (r == RPC_NO_ENCONTRADO) ? 0 : 1;
}

// ---------------------------------------------------------------
// Servicios (identicos a SERVER/server.c)
// ---------------------------------------------------------------

static FILE * abrir_archivo(void)
{
    FILE * fp = fopen(ARCHIVO, "rb+");
    if (fp == NULL)
        fp = fopen(ARCHIVO, "wb+");
    return fp;
}

static long contar_registros(FILE * fp)
{
    fseek(fp, 0, SEEK_END);
    return ftell(fp) / (long) sizeof(Persona);
}

int guardar_persona(Persona * p)
{
    FILE * fp;
    long total;
    int nuevo = (p->id == 0);

    fp = abrir_archivo();
    if (fp == NULL) {
        perror("Error opening file");
        return RPC_ERR_ARCHIVO;
    }

    total = contar_registros(fp);

    if (nuevo) {
        p->id = (int) total + 1;
    } else if (p->id < 0 || p->id > total) {
        fclose(fp);
        return RPC_NO_ENCONTRADO;
    }

    if (fseek(fp, (long)(p->id - 1) * (long) sizeof(Persona), SEEK_SET) != 0 ||
        fwrite(p, sizeof(Persona), 1, fp) != 1) {
        perror("Error writing file");
        if (nuevo) p->id = 0;
        fclose(fp);
        return RPC_ERR_ARCHIVO;
    }

    fclose(fp);
    return RPC_OK;
}

int recuperar_persona(Persona * p)
{
    Persona guardada;
    FILE * fp;

    if (p->id <= 0)
        return RPC_NO_ENCONTRADO;

    fp = fopen(ARCHIVO, "rb");
    if (fp == NULL)
        return RPC_NO_ENCONTRADO;

    if (fseek(fp, (long)(p->id - 1) * (long) sizeof(Persona), SEEK_SET) != 0 ||
        fread(&guardada, sizeof(Persona), 1, fp) != 1) {
        fclose(fp);
        return RPC_NO_ENCONTRADO;
    }

    fclose(fp);
    *p = guardada;
    return RPC_OK;
}
