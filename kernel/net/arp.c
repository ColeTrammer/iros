#include <arpa/inet.h>
#include <assert.h>

#include <kernel/net/arp.h>

void net_init_arp_packet(struct arp_packet *packet, uint16_t op, struct mac_address s_mac, struct ip_v4_address s_ip, struct mac_address t_mac, struct ip_v4_address t_ip) {
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