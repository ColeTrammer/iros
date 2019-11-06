#ifndef _KERNEL_NET_UNIX_SOCKET_H
#define _KERNEL_NET_UNIX_SOCKET_H 1

#include <kernel/net/socket.h>

int net_unix_bind(struct socket *socket, const struct sockaddr_un *addr, socklen_t addrlen);
int net_unix_socket(int domain, int type, int protocol);

#endif /* _KERNEL_NET_UNIX_SOCKET_H */