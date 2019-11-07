#ifndef _KERNEL_NET_UDP_H
#define _KERNEL_NET_UDP_H 1

#include <stdint.h>

#include <kernel/net/interface.h>

struct udp_packet {
    uint16_t source_port;
    uint16_t dest_port;
    uint16_t len;
    uint16_t checksum;
    uint8_t payload[0];
} __attribute__((packed));

ssize_t net_send_udp(struct network_interface *interface, struct ip_v4_address dest, uint16_t source_port, uint16_t dest_port, uint16_t len, const void *buf);
void net_udp_recieve(struct udp_packet *packet, size_t len);
void net_init_udp_packet(struct udp_packet *packet, uint16_t source_port, uint16_t dest_port, uint16_t len, const void *buf);

#endif /* _KERNEL_NET_UDP_H */