#include <arpa/inet.h>
#include <string.h>

#include <kernel/net/arp.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/ip.h>

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
