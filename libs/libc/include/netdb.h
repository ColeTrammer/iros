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

#define IPPORT_RESERVED 1024

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct hostent {
    char *h_name;
    char **h_aliases;
    int h_addrtype;
    int h_length;
    char **h_addr_list;
#define h_addr h_addr_list[0]
};

struct netent {
    char *n_name;
    char **n_aliases;
    int n_addrtype;
    uint32_t n_net;
};

struct protoent {
    char *p_name;
    char **p_aliases;
    int p_proto;
};

struct servent {
    char *s_name;
    char **s_aliases;
    int s_port;
    char *s_proto;
};

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

extern int h_errno;

void endhostent(void);
struct hostent *gethostbyaddr(const void *addr, socklen_t addrlen, int type);
struct hostent *gethostbyname(const char *name);
struct hostent *gethostent(void);
int gethostent_r(struct hostent *ret, char *buf, size_t buflen, struct hostent **result, int *h_errnop);
void sethostent(int stayopen);

void endnetent(void);
struct netent *getnetbyaddr(uint32_t net, int type);
struct netent *getnetbyname(const char *name);
struct netent *getnetent(void);
int getnetent_r(struct netent *net, char *buf, size_t buflen, struct netent **result);
void setnetent(int stayopen);

void endprotoent(void);
struct protoent *getprotobyname(const char *name);
struct protoent *getprotobynumber(int protocol);
struct protoent *getprotoent(void);
int getprotoent_r(struct protoent *proto, char *buf, size_t buflen, struct protoent **result);
void setprotoent(int stayopen);

void endservent(void);
struct servent *getservbyname(const char *name, const char *proto);
struct servent *getservbyport(int port, const char *proto);
struct servent *getservent(void);
void setservent(int stayopen);

void freeaddrinfo(struct addrinfo *res);
const char *gai_strerror(int err);
int getaddrinfo(const char *__restrict node, const char *__restrict service, const struct addrinfo *__restrict hints,
                struct addrinfo **__restrict res);
int getnameinfo(const struct sockaddr *__restrict addr, socklen_t addrlen, char *__restrict host, socklen_t hostlen, char *__restrict serv,
                socklen_t servlen, int flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _NETDB_H */
