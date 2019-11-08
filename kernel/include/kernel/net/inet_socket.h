#ifndef _KERNEL_NET_INET_SOCKET_H
#define _KERNEL_NET_INET_SOCKET_H 1

#include <netinet/in.h>

#include <kernel/net/ip_address.h>
#include <kernel/net/ip.h>
#include <kernel/net/socket.h>

struct inet_socket_data {
    uint16_t source_port;
};

struct socket_data *net_inet_create_socket_data(const struct ip_v4_packet *packet, uint16_t port_network_ordered, const void *buf, size_t len);

int net_inet_bind(struct socket *socket, const struct sockaddr_in *addr, socklen_t addrlen);
int net_inet_close(struct socket *socket);
int net_inet_socket(int domain, int type, int protocol);

ssize_t net_inet_sendto(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr_in *dest, socklen_t addrlen);
ssize_t net_inet_recvfrom(struct socket *socket, void *buf, size_t len, int flags, struct sockaddr_in *source, socklen_t *addrlen);

#endif /* _KERNEL_NET_INET_SOCKET_H */