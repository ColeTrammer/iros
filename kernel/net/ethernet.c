#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/arp.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/ip.h>
#include <kernel/net/network_task.h>
#include <kernel/net/packet.h>

uint16_t net_packet_header_to_ether_type(enum packet_header_type type) {
    switch (type) {
        case PH_ARP:
            return ETHERNET_TYPE_ARP;
        case PH_IP_V4:
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

void net_ethernet_recieve(struct packet *packet) {
    struct packet_header *header = net_packet_inner_header(packet);
    if (header->length < sizeof(struct ethernet_frame)) {
        debug_log("Ethernet frame too small\n");
        return;
    }

    struct packet_header *next_header = net_packet_add_header(packet, sizeof(struct ethernet_frame));

    struct ethernet_frame *frame = header->raw_header;
    switch (ntohs(frame->ether_type)) {
        case ETHERNET_TYPE_ARP:
            next_header->type = PH_ARP;
            net_arp_recieve(packet);
            break;
        case ETHERNET_TYPE_IPV4:
            next_header->type = PH_IP_V4;
            net_ip_v4_recieve(packet);
            break;
        default:
            debug_log("Recived unknown packet: [ %#4X ]\n", ntohs(frame->ether_type));
            break;
    }
}

void net_init_ethernet_frame(struct ethernet_frame *frame, struct mac_address dest, struct mac_address source, uint16_t type) {
    frame->mac_destination = dest;
    frame->mac_source = source;
    frame->ether_type = htons(type);
}
