#ifndef _KERNEL_NET_IP_H
#define _KERNEL_NET_IP_H 1

#include <stddef.h>
#include <stdint.h>

#include <kernel/net/ip_address.h>

#define IP_V4_VERSION 4

#define IP_V4_BYTES_TO_WORDS(bytes) ((bytes) / sizeof(uint32_t))

#define IP_V4_DONT_FRAGMENT  (1U << 1U)
#define IP_V4_MORE_FRAGMENTS (1U << 2U)

#define IP_V4_PROTOCOL_ICMP 0x01
#define IP_V4_PROTOCOL_TCP  0x06
#define IP_V4_PROTOCOL_UDP  0x11

struct ip_v4_packet {
    uint8_t version_and_ihl;
    uint8_t dscp_and_ecn;
    uint16_t length;
    uint16_t identification;
    uint16_t flags_and_fragment_offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;

    struct ip_v4_address source;
    struct ip_v4_address destination;

    uint8_t payload[0];
} __attribute__((packed));

uint16_t net_ip_v4_compute_checksum(void *packet, size_t num_bytes);
void net_init_ip_v4_packet(struct ip_v4_packet *packet, uint16_t ident, uint8_t protocol, struct ip_v4_address source, struct ip_v4_address dest, uint16_t payload_length);

#endif /* _KERNEL_NET_IP_H */