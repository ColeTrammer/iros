#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H 1

#include <sys/types.h>

#define SOCK_TYPE_MASK 0xFF
#define SOCK_DGRAM     1
#define SOCK_RAW       2
#define SOCK_SEQPACKET 3
#define SOCK_STREAM    4
#define SOCK_NONBLOCK  0x100

#define AF_UNSPEC 0
#define AF_INET   1
#define AF_INET6  2
#define AF_UNIX   3
#define AF_LOCAL  AF_UNIX

#define SHUT_RD   1
#define SHUT_RDWR 2
#define SHUT_WR   3

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef unsigned short sa_family_t;

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[14];
};

int accept(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict addrlen);
int bind(int fd, const struct sockaddr *addr, socklen_t addrlen);
int connect(int fd, const struct sockaddr *addr, socklen_t addrlen);
int listen(int fd, int backlog);
int socket(int domain, int type, int protocol);
int shutdown(int fd, int how);

int getsockopt(int fd, int level, int optname, void *__restrict optval, socklen_t *__restrict optlen);
int setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen);

int getpeername(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict addrlen);
int getsockname(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict addrlen);

ssize_t send(int fd, const void *buf, size_t len, int flags);
ssize_t sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *dest, socklen_t addrlen);

ssize_t recv(int fd, void *buf, size_t len, int flags);
ssize_t recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *__restrict src_addr, socklen_t *__restrict addrlen);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_SOCKET_H */