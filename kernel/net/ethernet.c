#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/net/arp.h>
#include <kernel/net/destination_cache.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/ip.h>
#include <kernel/net/neighbor_cache.h>
#include <kernel/net/network_task.h>

int net_ethernet_interface_send_arp(struct network_interface *self, struct link_layer_address dest_mac, struct network_data *data) {
    int ret = self->ops->send_ethernet(self, net_link_layer_address_to_mac(dest_mac), ETHERNET_TYPE_ARP, data->arp_packet, data->len);
    free(data);
    return ret;
}

int net_ethernet_interface_send_ip_v4(struct network_interface *self, struct destination_cache_entry *destination,
                                      struct network_data *data) {
    struct mac_address dest_mac = MAC_BROADCAST;
    if (destination) {
        if (destination->next_hop->state != NS_REACHABLE) {
            debug_log("No mac address found for ip: [ %d.%d.%d.%d ]\n", destination->next_hop->ip_v4_address.addr[0],
                      destination->next_hop->ip_v4_address.addr[1], destination->next_hop->ip_v4_address.addr[2],
                      destination->next_hop->ip_v4_address.addr[3]);
            return -EHOSTUNREACH;
        }
        dest_mac = net_link_layer_address_to_mac(destination->next_hop->link_layer_address);
    }

    int ret = self->ops->send_ethernet(self, dest_mac, ETHERNET_TYPE_IPV4, data->ip_v4_packet, data->len);
    net_free_network_data(data);
    return ret;
}

struct link_layer_address net_ethernet_interface_get_link_layer_broadcast_address(struct network_interface *self) {
    (void) self;
    return net_mac_to_link_layer_address(MAC_BROADCAST);
}

void net_recieve_ethernet(struct network_interface *interface, const struct ethernet_frame *frame, size_t len) {
    (void) interface;
    net_on_incoming_ethernet_frame(frame, len);
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
