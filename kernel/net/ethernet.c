#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/arp.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/ip.h>
#include <kernel/net/network_task.h>

uint16_t net_network_data_to_ether_type(enum network_data_type type) {
    switch (type) {
        case NETWORK_DATA_ARP:
            return ETHERNET_TYPE_ARP;
        case NETWORK_DATA_IP_V4:
            return ETHERNET_TYPE_IPV4;
        default:
            debug_log("Unkown conversion requested from network data type to ether type: [ %d ]\n", type);
            return 0;
    }
}

struct link_layer_address net_ethernet_interface_get_link_layer_broadcast_address(struct network_interface *self) {
    (void) self;
    return net_mac_to_link_layer_address(MAC_BROADCAST);
}

void net_recieve_ethernet(struct network_interface *interface, const struct ethernet_frame *frame, size_t len) {
    (void) interface;
    net_on_incoming_ethernet_frame(frame, interface, len);
}

void net_ethernet_recieve(const struct ethernet_frame *frame, size_t len) {
    if (len < sizeof(struct ethernet_frame)) {
        debug_log("Ethernet frame too small\n");
        return;
    }

    switch (ntohs(frame->ether_type)) {
        case ETHERNET_TYPE_ARP:
            net_arp_recieve((const struct arp_packet *) frame->payload, len - sizeof(struct ethernet_frame));
            break;
        case ETHERNET_TYPE_IPV4:
            net_ip_v4_recieve((const struct ip_v4_packet *) frame->payload, len - sizeof(struct ethernet_frame));
            break;
        default:
            debug_log("Recived unknown packet: [ %#4X ]\n", ntohs(frame->ether_type));
    }
}

void net_init_ethernet_frame(struct ethernet_frame *frame, struct mac_address dest, struct mac_address source, uint16_t type,
                             const void *payload, size_t payload_length) {
    frame->mac_destination = dest;
    frame->mac_source = source;
    frame->ether_type = htons(type);
    memcpy(frame->payload, payload, payload_length);
}
