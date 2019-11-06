#ifndef _KERNEL_NET_UNIX_SOCKET_H
#define _KERNEL_NET_UNIX_SOCKET_H 1

#include <kernel/net/socket.h>

struct unix_socket_data {
    char bound_path[UNIX_PATH_MAX];
    int connected_id;
};

int net_unix_accept(struct socket *socket, struct sockaddr_un *addr, socklen_t *addrlen);
int net_unix_bind(struct socket *socket, const struct sockaddr_un *addr, socklen_t addrlen);
int net_unix_close(struct socket *socket);
int net_unix_connect(struct socket *socket, const struct sockaddr_un *addr, socklen_t addrlen);
int net_unix_socket(int domain, int type, int protocol);

ssize_t net_unix_recv(struct socket *socket, void *buf, size_t len);
ssize_t net_unix_send(struct socket *socket, const void *buf, size_t len);

#endif /* _KERNEL_NET_UNIX_SOCKET_H */