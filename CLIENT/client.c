#include "client.h"

int sock;
char * host;

static void copy_string(char * dst, const char * src)
{
    strncpy(dst, src, MAX_STR - 1);
    dst[MAX_STR - 1] = '\0';
}

static void init_person(Person * p, const char * name,
                        const char * street, int number,
                        const char * city)
{
    memset(p, 0, sizeof(Person));
    p->id = 0;
    copy_string(p->name, name);
    copy_string(p->address.street, street);
    p->address.number = number;
    copy_string(p->address.city, city);
}

static void print_person(const char * title, const Person * p)
{
    printf("%s\n", title);
    printf("    id      : %d\n", p->id);
    printf("    name    : %s\n", p->name);
    printf("    address : %s #%d, %s\n\n",
           p->address.street, p->address.number, p->address.city);
}

static int demo(Person * p)
{
    Person q;
    int r;

    print_person("[1] Structure initialized on the client:", p);

    r = save_person(p);
    printf("    save_person() -> %d\n\n", r);
    if (r != RPC_OK) return r;
    print_person("[2] Structure after the call (id assigned by the server):", p);

    copy_string(p->address.street, "5th of May Street");
    p->address.number = 45;
    copy_string(p->address.city, "Cholula");
    print_person("[3] Structure modified on the client:", p);

    r = save_person(p);
    printf("    save_person() -> %d (record updated)\n\n", r);
    if (r != RPC_OK) return r;

    memset(&q, 0, sizeof(Person));
    q.id = p->id;
    r = retrieve_person(&q);
    printf("    retrieve_person(id=%d) -> %d\n\n", p->id, r);
    if (r != RPC_OK) return r;
    print_person("[4] Structure retrieved from the server's file:", &q);

    memset(&q, 0, sizeof(Person));
    q.id = 9999;
    r = retrieve_person(&q);
    printf("[5] retrieve_person(id=9999) -> %d (%s)\n", r,
           r == RPC_NOT_FOUND ? "not found, as expected"
                               : "unexpected result");

    return (r == RPC_NOT_FOUND) ? RPC_OK : RPC_PROTOCOL_ERROR;
}

int main( int argc, char * argv[] )
{
    Person p;
    int result;

    if( argc != 2 && argc != 6 ) {
        fprintf(stderr, "Usage: %s <host> [<name> <street> <number> <city>]\n",
                argv[0]);
        exit( 1 );
    }

    host = argv[1];

    if (argc == 6)
        init_person(&p, argv[2], argv[3], atoi(argv[4]), argv[5]);
    else
        init_person(&p, "John Smith", "Main Street", 123, "Springfield");

    sock = connection();

    result = demo(&p);

    close_socket(sock);
    return (result == RPC_OK) ? 0 : 1;
}
