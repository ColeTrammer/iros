#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/net/arp.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/interface.h>

void net_send_arp_request(struct network_interface *interface, struct ip_v4_address ip_address) {
    struct ethernet_packet *packet =
        net_create_ethernet_packet(MAC_BROADCAST, interface->ops->get_mac_address(interface), ETHERNET_TYPE_ARP, sizeof(struct arp_packet));

    net_init_arp_packet((struct arp_packet *) packet->payload, ARP_OPERATION_REQUEST, interface->ops->get_mac_address(interface),
                        interface->address, MAC_BROADCAST, ip_address);

    debug_log("Sending ARP packet for: [ %u.%u.%u.%u ]\n", ip_address.addr[0], ip_address.addr[1], ip_address.addr[2], ip_address.addr[3]);

    interface->ops->send(interface, packet, sizeof(struct ethernet_packet) + sizeof(struct arp_packet));

    free(packet);
}

void net_arp_recieve(const struct arp_packet *packet, size_t len) {
    assert(ntohs(packet->operation) == ARP_OPERATION_REPLY);

    if (len < sizeof(struct arp_packet)) {
        debug_log("ARP packet too small\n");
        return;
    }

    debug_log("Recieved ARP packet\n");

    debug_log("Updating IPV4 to MAC mapping: [ %u.%u.%u.%u, %02x:%02x:%02x:%02x:%02x:%02x ]\n", packet->ip_sender.addr[0],
              packet->ip_sender.addr[1], packet->ip_sender.addr[2], packet->ip_sender.addr[3], packet->mac_sender.addr[0],
              packet->mac_sender.addr[1], packet->mac_sender.addr[2], packet->mac_sender.addr[3], packet->mac_sender.addr[4],
              packet->mac_sender.addr[5]);

    struct ip_v4_to_mac_mapping *mapping = net_get_mac_from_ip_v4(packet->ip_sender);
    if (mapping) {
        mapping->mac = packet->mac_sender;
    } else {
        net_create_ip_v4_to_mac_mapping(packet->ip_sender, packet->mac_sender);
    }
}

void net_init_arp_packet(struct arp_packet *packet, uint16_t op, struct mac_address s_mac, struct ip_v4_address s_ip,
                         struct mac_address t_mac, struct ip_v4_address t_ip) {
    assert(packet);

    packet->hardware_type = htons(ARP_PROTOCOL_TYPE_ETHERNET);
    packet->protocol_type = htons(ARP_PROTOCOL_TYPE_IP_V4);
    packet->hardware_addr_len = sizeof(struct mac_address);
    packet->protocol_addr_len = sizeof(struct ip_v4_address);
    packet->operation = htons(op);
    packet->mac_sender = s_mac;
    packet->ip_sender = s_ip;
    packet->mac_target = t_mac;
    packet->ip_target = t_ip;
}
