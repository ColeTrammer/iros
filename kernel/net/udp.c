#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/net/destination_cache.h>
#include <kernel/net/dhcp.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/inet_socket.h>
#include <kernel/net/interface.h>
#include <kernel/net/ip.h>
#include <kernel/net/network_task.h>
#include <kernel/net/packet.h>
#include <kernel/net/port.h>
#include <kernel/net/socket.h>
#include <kernel/net/udp.h>
#include <kernel/util/checksum.h>
#include <kernel/util/macros.h>

int net_send_udp_through_socket(struct socket *socket, const void *buf, size_t len, const struct sockaddr *dest) {
    uint16_t source_port = PORT_FROM_SOCKADDR(&socket->host_address);
    struct ip_v4_address dest_ip = IP_V4_FROM_SOCKADDR(dest);
    uint16_t dest_port = PORT_FROM_SOCKADDR(dest);

    struct network_interface *interface = net_get_interface_for_ip(dest_ip);
    assert(interface);

    if (len > UINT16_MAX) {
        return -EMSGSIZE;
    }

    return net_send_udp(socket, interface, dest_ip, source_port, dest_port, len, buf);
}

int net_send_udp(struct socket *socket, struct network_interface *interface, struct ip_v4_address dest, uint16_t source_port,
                 uint16_t dest_port, uint16_t len, const void *buf) {
    if (interface->config_context.state != INITIALIZED) {
        debug_log("Can't send UDP packet; interface uninitialized: [ %s ]\n", interface->name);
        return -ENETDOWN;
    }

    struct destination_cache_entry *destination = net_lookup_destination(interface, dest);
    size_t udp_length = sizeof(struct udp_packet) + len;

    struct packet *packet = net_create_packet(interface, socket, destination, udp_length);
    packet->header_count = interface->link_layer_overhead + 2;

    net_drop_destination_cache_entry(destination);

    struct packet_header *udp_header =
        net_init_packet_header(packet, interface->link_layer_overhead + 1, PH_UDP, packet->inline_data, udp_length);
    struct udp_packet *udp_packet = udp_header->raw_header;
    net_init_udp_packet(udp_packet, source_port, dest_port, len, buf);

    struct ip_v4_pseudo_header header = { interface->address, dest, 0, IP_V4_PROTOCOL_UDP, htons(len + sizeof(struct udp_packet)) };
    udp_packet->checksum = ntohs(compute_partial_internet_checksum(udp_packet, sizeof(struct udp_packet) + len,
                                                                   compute_internet_checksum(&header, sizeof(struct ip_v4_pseudo_header))));

    debug_log("Sending UDP packet to: [ %u.%u.%u.%u, %u ]\n", dest.addr[0], dest.addr[1], dest.addr[2], dest.addr[3], dest_port);

    return interface->ops->route_ip_v4(interface, packet);
}

void net_udp_recieve(struct packet *net_packet) {
    struct packet_header *udp_header = net_packet_inner_header(net_packet);
    if (udp_header->length < sizeof(struct udp_packet)) {
        debug_log("UDP Packet to small\n");
        return;
    }

    struct packet_header *ip_header = net_packet_innermost_header_of_type(net_packet, PH_IP_V4);
    assert(ip_header);

    struct ip_v4_packet *ip_packet = ip_header->raw_header;
    struct udp_packet *packet = udp_header->raw_header;

    struct packet_header *next_header = net_packet_add_header(net_packet, sizeof(struct udp_packet));
    next_header->type = PH_RAW_DATA;

    uint16_t dest_port = ntohs(packet->dest_port);
    struct socket *socket = net_get_socket_from_port(dest_port);
    if (socket == NULL) {
        if (dest_port == DHCP_CLIENT_PORT) {
            net_dhcp_recieve(net_packet);
            return;
        }

        debug_log("UDP packet sent to unbound port: [ %u ]\n", dest_port);
        return;
    }

    struct socket_data *data = net_inet_create_socket_data(ip_packet, packet->source_port, next_header->raw_header, next_header->length);
    net_send_to_socket(socket, data);
}

void net_init_udp_packet(struct udp_packet *packet, uint16_t source_port, uint16_t dest_port, uint16_t len, const void *buf) {
    assert(packet);

    packet->source_port = htons(source_port);
    packet->dest_port = htons(dest_port);
    packet->len = htons(sizeof(struct udp_packet) + len);

    if (buf) {
        memcpy(packet->payload, buf, len);
    }

    packet->checksum = 0;
}
