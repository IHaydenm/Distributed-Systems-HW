#include "client.h"

extern int sock;

typedef struct {
    char data[MAX_MSG];
    int  len;
} Buffer;

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

static int put_u32(Buffer * b, uint32_t value)
{
    uint32_t net = htonl(value);
    if (b->len + (int) sizeof(net) > MAX_MSG) return -1;
    memcpy(b->data + b->len, &net, sizeof(net));
    b->len += (int) sizeof(net);
    return 0;
}

static int put_str(Buffer * b, const char * s)
{
    const char * end = (const char *) memchr(s, '\0', MAX_STR);
    uint32_t length;

    if (end == NULL) return -1;
    length = (uint32_t)(end - s) + 1;

    if (put_u32(b, length) < 0) return -1;
    if (b->len + (int) length > MAX_MSG) return -1;
    memcpy(b->data + b->len, s, length);
    b->len += (int) length;
    return 0;
}

static int put_person(Buffer * b, const Person * p)
{
    if (put_u32(b, (uint32_t) p->id) < 0)            return -1;
    if (put_str(b, p->name) < 0)                     return -1;
    if (put_str(b, p->address.street) < 0)           return -1;
    if (put_u32(b, (uint32_t) p->address.number) < 0) return -1;
    if (put_str(b, p->address.city) < 0)             return -1;
    return 0;
}

static int get_u32(int s, uint32_t * value)
{
    uint32_t net;
    if (recv_all(s, (char *) &net, sizeof(net)) < 0) return -1;
    *value = ntohl(net);
    return 0;
}

static int get_str(int s, char * dst)
{
    uint32_t length;

    if (get_u32(s, &length) < 0) return -1;
    if (length == 0 || length > MAX_STR) {
        puts("Protocol error: invalid string length");
        return -1;
    }
    if (recv_all(s, dst, (int) length) < 0) return -1;
    dst[length - 1] = '\0';
    return 0;
}

static int get_person(int s, Person * p)
{
    uint32_t id, number;

    memset(p, 0, sizeof(Person));

    if (get_u32(s, &id) < 0)                        return -1;
    p->id = (int) id;
    if (get_str(s, p->name) < 0)                    return -1;
    if (get_str(s, p->address.street) < 0)          return -1;
    if (get_u32(s, &number) < 0)                    return -1;
    p->address.number = (int) number;
    if (get_str(s, p->address.city) < 0)            return -1;
    return 0;
}

static int call_remote(uint32_t op, Person * p)
{
    Buffer   b;
    Person   received;
    uint32_t status;

    b.len = 0;

    if (put_u32(&b, op) < 0 || put_person(&b, p) < 0) {
        puts("Marshalling failed");
        return RPC_PROTOCOL_ERROR;
    }

    if (send_all(sock, b.data, b.len) < 0) {
        puts("Send request failed");
        return RPC_PROTOCOL_ERROR;
    }

    if (get_u32(sock, &status) < 0 || get_person(sock, &received) < 0)
        return RPC_PROTOCOL_ERROR;

    *p = received;

    return (int) status;
}

int save_person(Person * p)
{
    return call_remote(OP_SAVE, p);
}

int retrieve_person(Person * p)
{
    return call_remote(OP_RETRIEVE, p);
}
