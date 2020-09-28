#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H 1

#include <bits/sa_family_t.h>
#include <bits/size_t.h>
#include <bits/ssize_t.h>
#include <sys/uio.h>

#define SOCK_TYPE_MASK 0xFF
#define SOCK_DGRAM     1
#define SOCK_RAW       2
#define SOCK_SEQPACKET 3
#define SOCK_STREAM    4
#define SOCK_NONBLOCK  0x100
#define SOCK_CLOEXEC   0x200

#define AF_UNSPEC   0
#define AF_INET     1
#define AF_INET6    2
#define AF_UNIX     3
#define AF_LOCAL    AF_UNIX
#define AF_UMESSAGE 4

#define PF_UNSPEC   AF_UNSPEC
#define PF_INET     AF_INET
#define PF_INET6    AF_INET6
#define PF_UNIX     AF_UNIX
#define PF_LOCAL    PF_UNIX
#define PF_UMESSAGE AF_UMESSAGE

#define SHUT_RD   0
#define SHUT_WR   1
#define SHUT_RDWR 2

#define SOL_SOCKET 0

#define SO_ACCEPTCONN 1
#define SO_BROADCAST  2
#define SO_DEBUG      3
#define SO_DONTROUTE  4
#define SO_ERROR      5
#define SO_KEEPALIVE  6
#define SO_LINGER     7
#define SO_OOBINLINE  8
#define SO_RCVBUF     9
#define SO_RCVLOWAT   10
#define SO_RCVTIMEO   11
#define SO_REUSEADDR  12
#define SO_SNDBUF     13
#define SO_SNDLOWAT   14
#define SO_SNDTIMEO   15
#define SO_TYPE       16

#define SOMAXCONN 4096

#define MSG_CTRUNC    1
#define MSG_DONTROUTE 2
#define MSG_EOR       4
#define MSG_OOB       8
#define MSG_NOSIGNAL  16
#define MSG_PEEK      32
#define MSG_TRUNC     64
#define MSG_WAITALL   128

#define SCM_RIGHTS 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define __socklen_t_defined
#define __socklen_t unsigned int
typedef __socklen_t socklen_t;

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[14];
};

struct sockaddr_storage {
    sa_family_t ss_family;
    char __ss_pad1[sizeof(long long) - sizeof(sa_family_t)];
    long long __ss_align;
    char __ss_pad2[108 - 2 * sizeof(long long)]; /* UNIX_PATH_MAX == 108 */
};

struct msghdr {
    void *msg_name;
    socklen_t msg_namelen;
    struct iovec *msg_iov;
    int msg_iovlen;
    void *msg_control;
    socklen_t msg_controllen;
    int msg_flags;
};

struct cmsghdr {
    socklen_t cmsg_len;
    int cmsg_level;
    int cmsg_type;
};

#define CMSG_DATA(cmsg) ((unsigned char *) ((struct cmsghdr *) (cmsg) + 1))
#define CMSG_NXTHDR(mhdr, cmsg)                                                                                       \
    (((unsigned char *) (cmsg) - (unsigned char *) ((mhdr)->msg_control) + (cmsg)->cmsg_len) > (mhdr)->msg_controllen \
         ? NULL                                                                                                       \
         : ((struct cmsghdr *) (unsigned char *) (cmsg) + ((cmsg)->cmsg_len)))
#define CMSG_FIRSTHDR(mhdr) ((size_t)(mhdr)->msg_controllen < sizeof(struct cmsghdr) ? NULL : (struct cmsghdr *) ((mhdr)->msg_control))
#define CMSG_ALIGN(len)     ((((len) + sizeof(size_t) - 1) / sizeof(size_t)) * sizeof(size_t))
#define CMSG_SPACE(len)     (CMSG_ALIGN(len) + CMSG_ALIGN(sizeof(struct cmsghdr)))
#define CMSG_LEN(len)       (CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))

struct linger {
    int l_onoff;
    int l_linger;
};

int accept(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict addrlen);
int accept4(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict addrlen, int flags);
int bind(int fd, const struct sockaddr *addr, socklen_t addrlen);
int connect(int fd, const struct sockaddr *addr, socklen_t addrlen);
int getpeername(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict addrlen);
int getsockname(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict addrlen);
int getsockopt(int fd, int level, int optname, void *__restrict optval, socklen_t *__restrict optlen);
int listen(int fd, int backlog);
int setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen);
int shutdown(int fd, int how);
int sockatmark(int fd);
int socket(int domain, int type, int protocol);
int socketpair(int domain, int type, int protoco, int sv[2]);

ssize_t send(int fd, const void *buf, size_t len, int flags);
ssize_t sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *dest, socklen_t addrlen);
ssize_t sendmsg(int fd, const struct msghdr *message, int flags);

ssize_t recv(int fd, void *buf, size_t len, int flags);
ssize_t recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *__restrict src_addr, socklen_t *__restrict addrlen);
ssize_t recvmsg(int fd, struct msghdr *message, int flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_SOCKET_H */
