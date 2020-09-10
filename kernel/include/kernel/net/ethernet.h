#ifndef _KERNEL_NET_ETHERNET_H
#define _KERNEL_NET_ETHERNET_H 1

#include <stddef.h>
#include <stdint.h>

#include <kernel/net/mac.h>

#define ETHERNET_TYPE_IPV4 0x0800
#define ETHERNET_TYPE_ARP  0x0806

enum packet_header_type;

struct destination_cache_entry;
struct network_data;
struct network_interface;
struct packet;

struct ethernet_frame {
    struct mac_address mac_destination;
    struct mac_address mac_source;
    uint16_t ether_type;
    uint8_t payload[0];
} __attribute__((packed));

uint16_t net_packet_header_to_ether_type(enum packet_header_type type);

struct link_layer_address net_ethernet_interface_get_link_layer_broadcast_address(struct network_interface *self);
void net_recieve_ethernet(struct network_interface *interface, const struct ethernet_frame *frame, size_t len);

void net_ethernet_recieve(struct packet *packet);
void net_init_ethernet_frame(struct ethernet_frame *frame, struct mac_address dest, struct mac_address source, uint16_t type);

#endif /* _KERNEL_NET_ETHERNET_H */
