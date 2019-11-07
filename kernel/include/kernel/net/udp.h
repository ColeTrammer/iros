#ifndef _KERNEL_NET_UDP_H
#define _KERNEL_NET_UDP_H 1

#include <stdint.h>

struct udp_packet {
    uint16_t source_port;
    uint16_t dest_port;
    uint16_t len;
    uint16_t checksum;
    uint8_t payload[0];
} __attribute__((packed));

void net_init_udp_packet(struct udp_packet *packet, uint16_t source_port, uint16_t dest_port, uint16_t len, const void *buf);

#endif /* _KERNEL_NET_UDP_H */