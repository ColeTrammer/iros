#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/net/ethernet.h>
#include <kernel/net/interface.h>
#include <kernel/net/ip.h>
#include <kernel/net/udp.h>

ssize_t net_send_udp(struct network_interface *interface, struct ip_v4_address source, struct ip_v4_address dest, uint16_t source_port, uint16_t dest_port, uint16_t len, const void *buf) {
    size_t total_length = sizeof(struct ethernet_packet) + sizeof(struct ip_v4_packet) + sizeof(struct udp_packet) + len;

    struct ethernet_packet *packet = net_create_ethernet_packet(net_get_mac_from_ip_v4(interface->broadcast)->mac, interface->ops->get_mac_address(interface),
        ETHERNET_TYPE_ARP, total_length - sizeof(struct ethernet_packet));

    struct ip_v4_packet *ip_packet = (struct ip_v4_packet*) packet->payload;
    net_init_ip_v4_packet(ip_packet, 0, IP_V4_PROTOCOL_UDP, source, dest, total_length - sizeof(struct ethernet_packet) - sizeof(struct ip_v4_packet));

    struct udp_packet *udp_packet = (struct udp_packet*) ip_packet->payload;
    net_init_udp_packet(udp_packet, source_port, dest_port, len, buf);

    udp_packet->checksum = ntohs(in_compute_checksum(ip_packet, total_length - sizeof(struct ethernet_packet)));

    debug_log("Sending UDP packet to: [ %u.%u.%u.%u ]\n", dest.addr[0], dest.addr[1], dest.addr[2], dest.addr[3]);

    int ret = interface->ops->send(interface, packet, total_length);

    free(packet);
    return (ssize_t) ret;
}

void net_init_udp_packet(struct udp_packet *packet, uint16_t source_port, uint16_t dest_port, uint16_t len, const void *buf) {
    assert(packet);
    assert(buf);

    packet->source_port = htons(source_port);
    packet->dest_port = htons(dest_port);
    packet->len = htons(len);

    memcpy(packet->payload, buf, len);

    packet->checksum = 0;
}