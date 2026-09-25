#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STR 64

typedef struct {
    char street[MAX_STR];
    int number;
    char city[MAX_STR];
} Address;

typedef struct {
    int id;
    char name[MAX_STR];
    Address address;
} Person;

#define RPC_OK 0
#define RPC_NOT_FOUND  -1
#define RPC_FILE_ERROR -2

#define DATA_FILE "people.dat"

int save_person(Person * p);
int retrieve_person(Person * p);

static void copy_string(char * dst, const char * src){
    strncpy(dst, src, MAX_STR - 1);
    dst[MAX_STR - 1] = '\0';
}

static void init_person(Person * p, const char * name, const char * street, int number, const char * city){
    memset(p, 0, sizeof(Person));
    p->id = 0;
    copy_string(p->name, name);
    copy_string(p->address.street, street);
    p->address.number = number;
    copy_string(p->address.city, city);
}

static void print_person(const char * title, const Person * p){
    printf("%s\n", title);
    printf("    id      : %d\n", p->id);
    printf("    name    : %s\n", p->name);
    printf("    address : %s #%d, %s\n\n",
           p->address.street, p->address.number, p->address.city);
}

int main( int argc, char * argv[] ){
    Person p, q;
    int r;
    if( argc != 1 && argc != 5 ) {
        fprintf(stderr, "Usage: %s [<name> <street> <number> <city>]\n", argv[0]);
        exit( 1 );
    }

    if (argc == 5)
        init_person(&p, argv[1], argv[2], atoi(argv[3]), argv[4]);
    else
        init_person(&p, "John Smith", "Main Street", 123, "Springfield");
    print_person("[1] Structure initialized:", &p);

    r = save_person(&p);
    printf("    save_person() -> %d\n\n", r);
    if (r != RPC_OK) return 1;
    print_person("[2] Structure after the call (id assigned):", &p);

    copy_string(p.address.street, "5th of May Street");
    p.address.number = 45;
    copy_string(p.address.city, "Cholula");
    print_person("[3] Structure modified:", &p);

    r = save_person(&p);
    printf("    save_person() -> %d (record updated)\n\n", r);
    if (r != RPC_OK) return 1;

    memset(&q, 0, sizeof(Person));
    q.id = p.id;
    r = retrieve_person(&q);
    printf("    retrieve_person(id=%d) -> %d\n\n", p.id, r);
    if (r != RPC_OK) return 1;
    print_person("[4] Structure retrieved from the file:", &q);

    memset(&q, 0, sizeof(Person));
    q.id = 9999;
    r = retrieve_person(&q);
    printf("[5] retrieve_person(id=9999) -> %d (%s)\n", r,
           r == RPC_NOT_FOUND ? "not found, as expected"
                               : "unexpected result");
    return (r == RPC_NOT_FOUND) ? 0 : 1;
}

static FILE * open_file(void){
    FILE * fp = fopen(DATA_FILE, "rb+");
    if (fp == NULL)
        fp = fopen(DATA_FILE, "wb+");
    return fp;
}

static long count_records(FILE * fp){
    fseek(fp, 0, SEEK_END);
    return ftell(fp) / (long) sizeof(Person);
}

int save_person(Person * p){
    FILE * fp;
    long total;
    int is_new = (p->id == 0);
    fp = open_file();
    if (fp == NULL) {
        perror("Error opening file");
        return RPC_FILE_ERROR;
    }
    total = count_records(fp);

    if (is_new) {
        p->id = (int) total + 1;
    } else if (p->id < 0 || p->id > total) {
        fclose(fp);
        return RPC_NOT_FOUND;
    }

    if (fseek(fp, (long)(p->id - 1) * (long) sizeof(Person), SEEK_SET) != 0 ||
        fwrite(p, sizeof(Person), 1, fp) != 1) {
        perror("Error writing file");
        if (is_new) p->id = 0;
        fclose(fp);
        return RPC_FILE_ERROR;
    }

    fclose(fp);
    return RPC_OK;
}

int retrieve_person(Person * p){
    Person stored;
    FILE * fp;
    if (p->id <= 0)
        return RPC_NOT_FOUND;
    fp = fopen(DATA_FILE, "rb");
    if (fp == NULL)
        return RPC_NOT_FOUND;
    if (fseek(fp, (long)(p->id - 1) * (long) sizeof(Person), SEEK_SET) != 0 ||
        fread(&stored, sizeof(Person), 1, fp) != 1) {
        fclose(fp);
        return RPC_NOT_FOUND;
    }
    fclose(fp);
    *p = stored;
    return RPC_OK;
}
