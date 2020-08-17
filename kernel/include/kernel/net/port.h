#ifndef _KERNEL_NET_PORT_H
#define _KERNEL_NET_PORT_H 1

#include <stdint.h>

#include <kernel/util/hash_map.h>

struct port_to_socket_id {
    unsigned long socket_id;
    uint16_t port;
    struct hash_entry hash;
};

struct socket *net_get_socket_from_port(uint16_t port);

int net_bind_to_ephemeral_port(unsigned long socket_id, uint16_t *port_p);
int net_bind_to_port(unsigned long socket_id, uint16_t port);
void net_unbind_port(uint16_t port);

void init_ports();

#endif /* _KERNEL_NET_PORT_H */
