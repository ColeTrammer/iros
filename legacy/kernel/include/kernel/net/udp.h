#ifndef _KERNEL_NET_UDP_H
#define _KERNEL_NET_UDP_H 1

#include <stdint.h>

#include <kernel/net/interface.h>
#include <kernel/net/ip_address.h>

struct destination_cache_entry;
struct packet;
struct socket;

struct udp_packet {
    uint16_t source_port;
    uint16_t dest_port;
    uint16_t len;
    uint16_t checksum;
    uint8_t payload[0];
} __attribute__((packed));

int net_send_udp_through_socket(struct socket *socket, const void *buf, size_t len, const struct sockaddr *dest);
int net_send_udp(struct socket *socket, struct network_interface *interface, struct ip_v4_address dest, uint16_t source_port,
                 uint16_t dest_port, uint16_t len, const void *buf);
void net_udp_recieve(struct packet *packet);
void net_init_udp_packet(struct udp_packet *packet, uint16_t source_port, uint16_t dest_port, uint16_t len, const void *buf);

#endif /* _KERNEL_NET_UDP_H */
