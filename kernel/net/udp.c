#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/net/ethernet.h>
#include <kernel/net/interface.h>
#include <kernel/net/ip.h>
#include <kernel/net/port.h>
#include <kernel/net/socket.h>
#include <kernel/net/udp.h>

ssize_t net_send_udp(struct network_interface *interface, struct ip_v4_address dest, uint16_t source_port, uint16_t dest_port, uint16_t len, const void *buf) {
    size_t total_length = sizeof(struct ethernet_packet) + sizeof(struct ip_v4_packet) + sizeof(struct udp_packet) + len;

    struct ethernet_packet *packet = net_create_ethernet_packet(net_get_mac_from_ip_v4(interface->broadcast)->mac, interface->ops->get_mac_address(interface),
        ETHERNET_TYPE_IPV4, total_length - sizeof(struct ethernet_packet));

    struct ip_v4_packet *ip_packet = (struct ip_v4_packet*) packet->payload;
    net_init_ip_v4_packet(ip_packet, 1, IP_V4_PROTOCOL_UDP, interface->address, dest, total_length - sizeof(struct ethernet_packet) - sizeof(struct ip_v4_packet));

    struct udp_packet *udp_packet = (struct udp_packet*) ip_packet->payload;
    net_init_udp_packet(udp_packet, source_port, dest_port, len, buf);

    debug_log("Sending UDP packet to: [ %u.%u.%u.%u, %u ]\n", dest.addr[0], dest.addr[1], dest.addr[2], dest.addr[3], dest_port);

    ssize_t ret = interface->ops->send(interface, packet, total_length);

    free(packet);
    return ret < 0 ? ret : ret - (ssize_t) sizeof(struct ethernet_packet) - (ssize_t) sizeof(struct ip_v4_packet);
}

void net_udp_recieve(const struct udp_packet *packet, size_t len) {
    if (len < sizeof(struct udp_packet)) {
        debug_log("UDP Packet to small\n");
        return;
    }

    uint16_t dest_port = ntohs(packet->dest_port);
    struct socket *socket = net_get_socket_from_port(dest_port);
    if (socket == NULL) {
        debug_log("UDP packet sent to unbound port: [ %u ]\n", dest_port);
        return;
    }

    struct socket_data *data = calloc(1, sizeof(struct socket_data) + len);
    data->len = len;
    memcpy(data->data, packet->payload, len);
    net_send_to_socket(socket, data);
}

void net_init_udp_packet(struct udp_packet *packet, uint16_t source_port, uint16_t dest_port, uint16_t len, const void *buf) {
    assert(packet);
    assert(buf);

    packet->source_port = htons(source_port);
    packet->dest_port = htons(dest_port);
    packet->len = htons(sizeof(struct udp_packet) + len);

    memcpy(packet->payload, buf, len);

    packet->checksum = 0;
}