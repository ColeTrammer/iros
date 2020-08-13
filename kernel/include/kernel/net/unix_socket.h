#ifndef _KERNEL_NET_UNIX_SOCKET_H
#define _KERNEL_NET_UNIX_SOCKET_H 1

#include <kernel/net/socket.h>

struct unix_socket_data {
    int connected_id;
};

int net_unix_accept(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen, int flags);
int net_unix_bind(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen);
int net_unix_close(struct socket *socket);
int net_unix_connect(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen);
int net_unix_getpeername(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen);
int net_unix_socket(int domain, int type, int protocol);

ssize_t net_unix_recvfrom(struct socket *socket, void *buf, size_t len, int flags, struct sockaddr *source, socklen_t *addrlen);
ssize_t net_unix_sendto(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr *dest, socklen_t addrlen);

#endif /* _KERNEL_NET_UNIX_SOCKET_H */
