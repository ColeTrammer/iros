#ifndef _KERNEL_NET_ETHERNET_H
#define _KERNEL_NET_ETHERNET_H 1

#include <stddef.h>
#include <stdint.h>

#include <kernel/net/mac.h>

#define ETHERNET_TYPE_IPV4 0x0800
#define ETHERNET_TYPE_ARP  0x0806

struct ethernet_frame {
    struct mac_address mac_destination;
    struct mac_address mac_source;
    uint16_t ether_type;
    uint8_t payload[0];
} __attribute__((packed));

void net_ethernet_recieve(const struct ethernet_frame *frame, size_t len);
void net_init_ethernet_frame(struct ethernet_frame *frame, struct mac_address dest, struct mac_address source, uint16_t type,
                             const void *payload, size_t payload_length);

#endif /* _KERNEL_NET_ETHERNET_H */
