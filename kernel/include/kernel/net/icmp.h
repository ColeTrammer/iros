#ifndef _KERNEL_NET_ICMP_H
#define _KERNEL_NET_ICMP_H 1

#include <stdint.h>

#define ICMP_TYPE_ECHO_REPLY   0
#define ICMP_TYPE_ECHO_REQUEST 8

struct icmp_packet {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence_number;

    uint8_t payload[0];
} __attribute__((packed));

void net_icmp_recieve(const struct icmp_packet *packet, size_t len);

void net_init_icmp_packet(struct icmp_packet *packet, uint8_t type, uint16_t identifier, uint16_t sequence, void *payload, uint16_t payload_size);

#endif /* _KERNEL_NET_ICMP_H */