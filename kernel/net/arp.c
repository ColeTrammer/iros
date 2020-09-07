#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/net/arp.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/interface.h>
#include <kernel/net/network_task.h>

void net_send_arp_request(struct network_interface *interface, struct ip_v4_address ip_address) {
    if (interface->config_context.state != INITIALIZED) {
        debug_log("Can't send ARP packet; interface uninitialized: [ %s ]\n", interface->name);
        return;
    }

    struct network_data *data = net_create_arp_packet(ARP_OPERATION_REQUEST, interface->ops->get_link_layer_address(interface),
                                                      interface->address, net_mac_to_link_layer_address(MAC_BROADCAST), ip_address);

    debug_log("Sending ARP packet for: [ %u.%u.%u.%u ]\n", ip_address.addr[0], ip_address.addr[1], ip_address.addr[2], ip_address.addr[3]);

    interface->ops->send_arp(interface, net_mac_to_link_layer_address(MAC_BROADCAST), data);
}

void net_arp_recieve(const struct arp_packet *packet, size_t len) {
    assert(ntohs(packet->operation) == ARP_OPERATION_REPLY);

    if (len < sizeof(struct arp_packet)) {
        debug_log("ARP packet too small\n");
        return;
    }

    struct ip_v4_address *ip_sender = (struct ip_v4_address *) ARP_SENDER_PROTO_ADDR(packet);
    struct mac_address *mac_sender = (struct mac_address *) ARP_SENDER_HW_ADDR(packet);
    debug_log("Updating IPV4 to MAC mapping: [ %u.%u.%u.%u, %02x:%02x:%02x:%02x:%02x:%02x ]\n", ip_sender->addr[0], ip_sender->addr[1],
              ip_sender->addr[2], ip_sender->addr[3], mac_sender->addr[0], mac_sender->addr[1], mac_sender->addr[2], mac_sender->addr[3],
              mac_sender->addr[4], mac_sender->addr[5]);

    struct ip_v4_to_mac_mapping *mapping = net_get_mac_from_ip_v4(*ip_sender);
    if (mapping) {
        mapping->mac = *mac_sender;
    } else {
        net_create_ip_v4_to_mac_mapping(*ip_sender, *mac_sender);
    }
}

struct network_data *net_create_arp_packet(uint16_t op, struct link_layer_address s_addr, struct ip_v4_address s_ip,
                                           struct link_layer_address t_addr, struct ip_v4_address t_ip) {
    size_t arp_length = sizeof(struct arp_packet) + 2 * sizeof(struct ip_v4_address) + 2 * s_addr.length;
    struct network_data *data = malloc(sizeof(struct network_data) + arp_length);
    data->type = NETWORK_DATA_ARP;
    data->len = sizeof(struct arp_packet) + arp_length;
    data->arp_packet = (struct arp_packet *) (data + 1);
    net_init_arp_packet(data->arp_packet, op, s_addr, s_ip, t_addr, t_ip);
    return data;
}

void net_init_arp_packet(struct arp_packet *packet, uint16_t op, struct link_layer_address s_addr, struct ip_v4_address s_ip,
                         struct link_layer_address t_addr, struct ip_v4_address t_ip) {
    // FIXME: choose the hardware type properly.
    packet->hardware_type = htons(ARP_PROTOCOL_TYPE_ETHERNET);
    packet->protocol_type = htons(ARP_PROTOCOL_TYPE_IP_V4);
    packet->hardware_addr_len = s_addr.length;
    packet->protocol_addr_len = sizeof(struct ip_v4_address);
    packet->operation = htons(op);
    memcpy(ARP_SENDER_HW_ADDR(packet), s_addr.addr, s_addr.length);
    memcpy(ARP_SENDER_PROTO_ADDR(packet), s_ip.addr, sizeof(struct ip_v4_address));
    memcpy(ARP_TARGET_HW_ADDR(packet), t_addr.addr, t_addr.length);
    memcpy(ARP_TARGET_PROTO_ADDR(packet), t_ip.addr, sizeof(struct ip_v4_address));
}
