#ifndef _KERNEL_NET_INET_SOCKET_H
#define _KERNEL_NET_INET_SOCKET_H 1

#include <arpa/inet.h>
#include <netinet/in.h>

#include <kernel/net/ip.h>
#include <kernel/net/ip_address.h>
#include <kernel/net/socket.h>
#include <kernel/util/hash_map.h>

struct socket_data *net_inet_create_socket_data(const struct ip_v4_packet *packet, uint16_t port_network_ordered, const void *buf,
                                                size_t len);

int net_inet_bind(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen);
int net_inet_close(struct socket *socket);
int net_inet_connect(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen);
int net_inet_getpeername(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen);
int net_inet_getsockname(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen);
ssize_t net_inet_sendto(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen);

void init_inet_sockets();

#define PORT_FROM_SOCKADDR(s)  ntohs(((struct sockaddr_in *) (s))->sin_port)
#define IP_V4_FROM_SOCKADDR(s) ip_v4_from_uint(((struct sockaddr_in *) (s))->sin_addr.s_addr)

#endif /* _KERNEL_NET_INET_SOCKET_H */
