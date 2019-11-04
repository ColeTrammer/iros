#ifndef _KERNEL_NET_ETHERNET_H
#define _KERNEL_NET_ETHERNET_H 1

#include <stddef.h>
#include <stdint.h>

#include <kernel/net/mac.h>

#define ETHERNET_TYPE_IPV4 0x0800
#define ETHERNET_TYPE_ARP  0x0806

struct ethernet_packet {
    struct mac_address mac_destination;
    struct mac_address mac_source;
    uint16_t ether_type;
    uint8_t payload[0];
} __attribute__((packed));

struct ethernet_packet *net_create_ethernet_packet(struct mac_address dest, struct mac_address source, uint16_t type, size_t payload_size);

#endif /* _KERNEL_NET_ETHERNET_H */