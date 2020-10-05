#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/net/arp.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/interface.h>
#include <kernel/net/neighbor_cache.h>
#include <kernel/net/network_task.h>
#include <kernel/net/packet.h>

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
    struct link_layer_address broadcast_address = interface->ops->get_link_layer_broadcast_address(interface);
    struct packet *packet = net_create_arp_packet(interface, ARP_OPERATION_REQUEST, interface->link_layer_address, interface->address,
                                                  broadcast_address, ip_address);

    debug_log("Sending ARP packet for: [ %u.%u.%u.%u ]\n", ip_address.addr[0], ip_address.addr[1], ip_address.addr[2], ip_address.addr[3]);

    interface->ops->send(interface, broadcast_address, packet);
}

void net_arp_recieve(struct packet *packet) {
    struct packet_header *header = net_packet_inner_header(packet);
    if (header->length < sizeof(struct arp_packet)) {
        debug_log("ARP packet too small\n");
        return;
    }

    struct arp_packet *arp_packet = header->raw_header;
    if (ntohs(arp_packet->operation) != ARP_OPERATION_REPLY) {
        debug_log("Unsupported ARP operation: [ %u ]\n", ntohs(arp_packet->operation));
        return;
    }

    struct ip_v4_address ip_sender = net_arp_sender_proto_addr(arp_packet);
    struct link_layer_address hw_sender = net_arp_sender_hw_addr(arp_packet);

    struct neighbor_cache_entry *neighbor = net_lookup_neighbor(packet->interface, ip_sender);
    net_update_neighbor(neighbor, hw_sender);
    net_drop_neighbor_cache_entry(neighbor);
}

struct packet *net_create_arp_packet(struct network_interface *interface, uint16_t op, struct link_layer_address s_addr,
                                     struct ip_v4_address s_ip, struct link_layer_address t_addr, struct ip_v4_address t_ip) {
    size_t arp_length = sizeof(struct arp_packet) + 2 * sizeof(struct ip_v4_address) + 2 * s_addr.length;
    struct packet *packet = net_create_packet(interface, NULL, NULL, arp_length);
    packet->header_count = interface->link_layer_overhead + 1;

    struct packet_header *header = net_init_packet_header(packet, interface->link_layer_overhead, PH_ARP, packet->inline_data, arp_length);
    net_init_arp_packet(header->raw_header, op, s_addr, s_ip, t_addr, t_ip);
    return packet;
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
