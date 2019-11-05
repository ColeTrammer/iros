#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H 1

#include <sys/types.h>

#define SOCK_DGRAM     1
#define SOCK_RAW       2
#define SOCK_SEQPACKET 3
#define SOCK_STREAM    4

#define AF_UNSPEC 0
#define AF_INET   1
#define AF_INET6  2
#define AF_UNIX   3

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef unsigned short sa_family_t;

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[14];
};

int socket(int domain, int type, int protocol);

ssize_t send(int fd, const void *buf, size_t len, int flags);
ssize_t sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *dest, socklen_t addrlen);

ssize_t recv(int fd, void *buf, size_t len, int flags);
ssize_t recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_SOCKET_H */