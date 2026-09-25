/* server.c: Codigo de los procedimientos remotos
 *
 * Almacena y recupera estructuras Persona en el archivo "personas.dat".
 * Las funciones estan escritas como procedimientos locales: no contienen
 * ningun codigo de red (eso esta en adapter_svc.c y utils.c).
 *
 * Cada registro ocupa sizeof(Persona) bytes y su posicion en el archivo
 * es (id - 1): el id 1 es el primer registro, el 2 el segundo, etc.
 */

#include "server.h"

#define ARCHIVO "personas.dat"

// Abre el archivo para lectura/escritura; lo crea si aun no existe
static FILE * abrir_archivo(void)
{
    FILE * fp = fopen(ARCHIVO, "rb+");
    if (fp == NULL)
        fp = fopen(ARCHIVO, "wb+");
    return fp;
}

// Numero de registros almacenados
static long contar_registros(FILE * fp)
{
    fseek(fp, 0, SEEK_END);
    return ftell(fp) / (long) sizeof(Persona);
}

// Servicio "guardar_persona"
//
//   id == 0 : alta. El servidor MODIFICA la estructura asignandole un id
//             nuevo y agrega el registro al archivo.
//   id  > 0 : actualizacion del registro existente con ese id.
//
// Regresa RPC_OK, RPC_NO_ENCONTRADO (id inexistente) o RPC_ERR_ARCHIVO.

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
        p->id = (int) total + 1;                    // modifica el argumento
    } else if (p->id < 0 || p->id > total) {
        fclose(fp);
        return RPC_NO_ENCONTRADO;
    }

    if (fseek(fp, (long)(p->id - 1) * (long) sizeof(Persona), SEEK_SET) != 0 ||
        fwrite(p, sizeof(Persona), 1, fp) != 1) {
        perror("Error writing file");
        if (nuevo) p->id = 0;                       // deja la estructura como llego
        fclose(fp);
        return RPC_ERR_ARCHIVO;
    }

    fclose(fp);
    printf("guardar_persona: id=%d nombre=\"%s\"\n", p->id, p->nombre);
    return RPC_OK;
}

// Servicio "recuperar_persona"
//
// Usa p->id como llave y llena el resto de la estructura con lo que esta
// almacenado en el archivo. Si el id no existe, la estructura no cambia.
//
// Regresa RPC_OK o RPC_NO_ENCONTRADO.

int recuperar_persona(Persona * p)
{
    Persona guardada;
    FILE * fp;

    if (p->id <= 0)
        return RPC_NO_ENCONTRADO;

    fp = fopen(ARCHIVO, "rb");
    if (fp == NULL)                                 // todavia no hay archivo
        return RPC_NO_ENCONTRADO;

    if (fseek(fp, (long)(p->id - 1) * (long) sizeof(Persona), SEEK_SET) != 0 ||
        fread(&guardada, sizeof(Persona), 1, fp) != 1) {
        fclose(fp);
        return RPC_NO_ENCONTRADO;
    }

    fclose(fp);
    *p = guardada;                                  // modifica el argumento
    printf("recuperar_persona: id=%d nombre=\"%s\"\n", p->id, p->nombre);
    return RPC_OK;
}
