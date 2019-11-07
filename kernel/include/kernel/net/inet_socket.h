#ifndef _KERNEL_NET_INET_SOCKET_H
#define _KERNEL_NET_INET_SOCKET_H 1

#include <netinet/in.h>

#include <kernel/net/socket.h>

int net_inet_socket(int domain, int type, int protocol);

ssize_t net_inet_sendto(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr_in *dest, socklen_t addrlen);
ssize_t net_inet_recvfrom(struct socket *socket, void *buf, size_t len, int flags, struct sockaddr_in *source, socklen_t *addrlen);

#endif /* _KERNEL_NET_INET_SOCKET_H */