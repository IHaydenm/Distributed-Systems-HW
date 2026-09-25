PRACTICA RPC - Transferencia de una estructura compleja por referencia
======================================================================

Estructura (definida en CLIENT/client.h y SERVER/server.h):

    Persona { id, nombre, Direccion { calle, numero, ciudad } }

Procedimientos remotos (la estructura se pasa POR REFERENCIA, copy-restore):

    int guardar_persona(Persona *p);    id==0: alta, el servidor asigna el id
                                        id>0 : actualiza el registro
    int recuperar_persona(Persona *p);  usa p->id y llena el resto desde archivo

Codigos de retorno:  0 = OK, -1 = no encontrado, -2 = error de archivo,
                    -3 = error de comunicacion / protocolo

Organizacion:

    centralizada.c            Version centralizada (todo en un programa)
    CLIENT/
        client.c              Cliente: inicializa / modifica la estructura
        adapter_clnt.c        Adaptador del cliente (stub)
        utils.c, client.h
    SERVER/
        server.c              Servicios: almacenan y recuperan del archivo
        adapter_svc.c         Adaptador del servidor (skeleton) + main()
        utils.c, server.h

Prueba rapida (dos terminales):

    Terminal 1:  cd SERVER && cc server.c adapter_svc.c utils.c -o server && ./server
    Terminal 2:  cd CLIENT && cc client.c adapter_clnt.c utils.c -o client && ./client 127.0.0.1

Version centralizada:

    cc centralizada.c -o centralizada && ./centralizada

Formato de los mensajes (enteros de 4 bytes en Network Byte Order):

    Peticion : [op][id][nombre][calle][numero][ciudad]
    Respuesta: [status][id][nombre][calle][numero][ciudad]
    Cadena   : [longitud incluyendo '\0'][bytes]
