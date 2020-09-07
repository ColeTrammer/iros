#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/net/arp.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/interface.h>
#include <kernel/net/network_task.h>

static uint16_t net_ll_to_arp_hw_type(enum ll_address_type type) {
    switch (type) {
        case LL_TYPE_ETHERNET:
            return ARP_PROTOCOL_TYPE_ETHERNET;
        default:
            assert(false);
            return 0;
    }
}

static enum ll_address_type net_arp_hw_to_ll_type(uint16_t type) {
    switch (type) {
        case ARP_PROTOCOL_TYPE_ETHERNET:
            return LL_TYPE_ETHERNET;
        default:
            debug_log("Unkown ARP link layer type: [ %u ]\n", type);
            return LL_TYPE_NONE;
    }
}

static struct link_layer_address net_arp_sender_hw_addr(const struct arp_packet *packet) {
    struct link_layer_address ret;
    ret.type = net_arp_hw_to_ll_type(ntohs(packet->hardware_type));
    ret.length = packet->hardware_addr_len;
    memcpy(ret.addr, ARP_SENDER_HW_ADDR(packet), ret.length);
    return ret;
}

static struct ip_v4_address net_arp_sender_proto_addr(const struct arp_packet *packet) {
    return *(struct ip_v4_address *) ARP_SENDER_PROTO_ADDR(packet);
}

static __attribute__((unused)) struct link_layer_address net_arp_target_hw_addr(const struct arp_packet *packet) {
    struct link_layer_address ret;
    ret.type = net_arp_hw_to_ll_type(ntohs(packet->hardware_type));
    ret.length = packet->hardware_addr_len;
    memcpy(ret.addr, ARP_TARGET_HW_ADDR(packet), ret.length);
    return ret;
}

static __attribute__((unused)) struct ip_v4_address net_arp_target_proto_addr(const struct arp_packet *packet) {
    return *(struct ip_v4_address *) ARP_TARGET_PROTO_ADDR(packet);
}

void net_send_arp_request(struct network_interface *interface, struct ip_v4_address ip_address) {
    if (interface->config_context.state != INITIALIZED) {
        debug_log("Can't send ARP packet; interface uninitialized: [ %s ]\n", interface->name);
        return;
    }

    struct link_layer_address broadcast_address = interface->ops->get_link_layer_broadcast_address(interface);
    struct network_data *data = net_create_arp_packet(ARP_OPERATION_REQUEST, interface->ops->get_link_layer_address(interface),
                                                      interface->address, broadcast_address, ip_address);

    debug_log("Sending ARP packet for: [ %u.%u.%u.%u ]\n", ip_address.addr[0], ip_address.addr[1], ip_address.addr[2], ip_address.addr[3]);

    interface->ops->send_arp(interface, broadcast_address, data);
}

void net_arp_recieve(const struct arp_packet *packet, size_t len) {
    assert(ntohs(packet->operation) == ARP_OPERATION_REPLY);

    if (len < sizeof(struct arp_packet)) {
        debug_log("ARP packet too small\n");
        return;
    }

    struct ip_v4_address ip_sender = net_arp_sender_proto_addr(packet);
    struct link_layer_address hw_sender = net_arp_sender_hw_addr(packet);
    struct mac_address mac_sender = net_link_layer_address_to_mac(hw_sender);
    debug_log("Updating IPV4 to MAC mapping: [ %u.%u.%u.%u, %02x:%02x:%02x:%02x:%02x:%02x ]\n", ip_sender.addr[0], ip_sender.addr[1],
              ip_sender.addr[2], ip_sender.addr[3], mac_sender.addr[0], mac_sender.addr[1], mac_sender.addr[2], mac_sender.addr[3],
              mac_sender.addr[4], mac_sender.addr[5]);

    struct ip_v4_to_mac_mapping *mapping = net_get_mac_from_ip_v4(ip_sender);
    if (mapping) {
        mapping->mac = mac_sender;
    } else {
        net_create_ip_v4_to_mac_mapping(ip_sender, mac_sender);
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
    packet->hardware_type = htons(net_ll_to_arp_hw_type(s_addr.type));
    packet->protocol_type = htons(ARP_PROTOCOL_TYPE_IP_V4);
    packet->hardware_addr_len = s_addr.length;
    packet->protocol_addr_len = sizeof(struct ip_v4_address);
    packet->operation = htons(op);
    memcpy(ARP_SENDER_HW_ADDR(packet), s_addr.addr, s_addr.length);
    memcpy(ARP_SENDER_PROTO_ADDR(packet), s_ip.addr, sizeof(struct ip_v4_address));
    memcpy(ARP_TARGET_HW_ADDR(packet), t_addr.addr, t_addr.length);
    memcpy(ARP_TARGET_PROTO_ADDR(packet), t_ip.addr, sizeof(struct ip_v4_address));
}
