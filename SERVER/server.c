#include "server.h"

#define DATA_FILE "people.dat"

static FILE * open_file(void)
{
    FILE * fp = fopen(DATA_FILE, "rb+");
    if (fp == NULL)
        fp = fopen(DATA_FILE, "wb+");
    return fp;
}

static long count_records(FILE * fp)
{
    fseek(fp, 0, SEEK_END);
    return ftell(fp) / (long) sizeof(Person);
}

int save_person(Person * p)
{
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
    printf("save_person: id=%d name=\"%s\"\n", p->id, p->name);
    return RPC_OK;
}

int retrieve_person(Person * p)
{
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
    printf("retrieve_person: id=%d name=\"%s\"\n", p->id, p->name);
    return RPC_OK;
}
