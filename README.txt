RPC PRACTICE - Passing a complex structure by reference
=========================================================

Structure (defined in CLIENT/client.h and SERVER/server.h):

    Person { id, name, Address { street, number, city } }

Remote procedures (the structure is passed BY REFERENCE, copy-restore):

    int save_person(Person *p);     id==0: create, the server assigns the id
                                    id>0 : updates the existing record
    int retrieve_person(Person *p); uses p->id and fills in the rest from the file

Return codes:  0 = OK, -1 = not found, -2 = file error,
              -3 = communication / protocol error

Layout:

    centralized.c              Centralized version (everything in one program)
    CLIENT/
        client.c              Client: initializes / modifies the structure
        adapter_clnt.c        Client-side adapter (stub)
        utils.c, client.h
    SERVER/
        server.c              Services: store and retrieve from the file
        adapter_svc.c         Server-side adapter (skeleton) + main()
        utils.c, server.h

Quick test (two terminals):

    Terminal 1:  cd SERVER && cc server.c adapter_svc.c utils.c -o server && ./server
    Terminal 2:  cd CLIENT && cc client.c adapter_clnt.c utils.c -o client && ./client 127.0.0.1

Centralized version:

    cc centralized.c -o centralized && ./centralized

Message format (4-byte integers in Network Byte Order):

    Request : [op][id][name][street][number][city]
    Response: [status][id][name][street][number][city]
    String  : [length including '\0'][bytes]
