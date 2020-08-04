#ifndef _NETDB_H
#define _NETDB_H 1

#include <netinet/in.h>

#define AI_PASSIVE      1
#define AI_CANONNAME    2
#define AI_NUMERIC_HOST 4
#define AI_NUMERICSERV  8
#define AI_V4MAPPED     16
#define AI_ALL          32
#define AI_ADDRCONFIG   64

#define NI_NOFQDN       1
#define NI_NUMERICHOST  2
#define NI_NAMEREQD     4
#define NI_NUMERICSERV  8
#define NI_NUMERICSCOPE 16
#define NI_DGRAM        32

#define EAI_AGAIN    -1
#define EAI_BADFLAGS -2
#define EAI_FAIL     -3
#define EAI_FAMILY   -4
#define EAI_MEMORY   -5
#define EAI_NONAME   -6
#define EAI_SERVICE  -7
#define EAI_SOCKTYPE -8
#define EAI_SYSTEM   -9
#define EAI_OVERFLOW -10

#define HOST_NOT_FOUND -1
#define NO_DATA        -2
#define NO_RECOVERY    -3
#define TRY_AGAIN      -4

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
