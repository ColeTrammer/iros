#ifndef _NETDB_H
#define _NETDB_H 1

#include <netinet/in.h>

#define EAI_NONAME  -1
#define EAI_FAMILY  -2
#define EAI_SYSTEM  -3
#define EAI_SERVICE -4

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct addrinfo {
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    socklen_t ai_addrlen;
    struct sockaddr *ai_addr;
    char *ai_canonname;
    struct addrinfo *ai_next;
};

int getaddrinfo(const char *__restrict node, const char *__restrict service, const struct addrinfo *__restrict hints,
                struct addrinfo **__restrict res);
void freeaddrinfo(struct addrinfo *res);

int getnameinfo(const struct sockaddr *__restrict addr, socklen_t addrlen, char *__restrict host, socklen_t hostlen, char *__restrict serv,
                socklen_t servlen, int flags);

const char *gai_strerror(int err);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _NETDB_H */