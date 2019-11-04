#include <assert.h>
#include <stdlib.h>

#include <kernel/net/arp.h>

struct arp_packet *net_create_arp_packet(uint16_t op, struct mac_address s_mac, struct ip_v4_address s_ip, struct mac_address t_mac, struct ip_v4_address t_ip) {
    struct arp_packet *packet = malloc(sizeof(struct arp_packet));
    assert(packet);

    packet->hardware_type = ARP_PROTOCOL_TYPE_ETHERNET;
    packet->protocol_type = ARP_PROTOCOL_TYPE_IP_V4;
    packet->hardware_addr_len = sizeof(struct mac_address);
    packet->protocol_addr_len = sizeof(struct ip_v4_address);
    packet->operation = op;
    packet->mac_sender = s_mac;
    packet->ip_sender = s_ip;
    packet->mac_target = t_mac;
    packet->ip_target = t_ip;

    return packet;
}