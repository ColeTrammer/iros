#ifndef _KERNEL_NET_PORT_H
#define _KERNEL_NET_PORT_H 1

#include <stdint.h>

#include <kernel/util/hash_map.h>

struct socket;

struct port_to_socket {
    struct socket *socket;
    uint16_t port;
    struct hash_entry hash;
};

struct socket *net_get_socket_from_port(uint16_t port);

int net_bind_to_ephemeral_port(struct socket *socket, uint16_t *port_p);
int net_bind_to_port(struct socket *socket, uint16_t port);
void net_unbind_port(uint16_t port);

#endif /* _KERNEL_NET_PORT_H */
