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
#include <kernel/net/port.h>
#include <kernel/net/socket.h>
#include <kernel/net/udp.h>
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
    size_t total_length = sizeof(struct ip_v4_packet) + sizeof(struct udp_packet) + len;

    struct network_data *data =
        net_create_ip_v4_packet(socket, 1, IP_V4_PROTOCOL_UDP, interface->address, dest, NULL, total_length - sizeof(struct ip_v4_packet));

    struct udp_packet *udp_packet = (struct udp_packet *) data->ip_v4_packet->payload;
    net_init_udp_packet(udp_packet, source_port, dest_port, len, buf);

    struct ip_v4_pseudo_header header = { interface->address, dest, 0, IP_V4_PROTOCOL_UDP, htons(len + sizeof(struct udp_packet)) };

    udp_packet->checksum = ntohs(in_compute_checksum_with_start(udp_packet, sizeof(struct udp_packet) + len,
                                                                in_compute_checksum(&header, sizeof(struct ip_v4_pseudo_header))));

    debug_log("Sending UDP packet to: [ %u.%u.%u.%u, %u ]\n", dest.addr[0], dest.addr[1], dest.addr[2], dest.addr[3], dest_port);

    int ret = interface->ops->send_ip_v4(interface, destination, data);

    net_drop_destination_cache_entry(destination);
    return ret;
}

void net_udp_recieve(const struct udp_packet *packet, size_t len) {
    if (len < sizeof(struct udp_packet)) {
        debug_log("UDP Packet to small\n");
        return;
    }

    uint16_t dest_port = ntohs(packet->dest_port);
    struct socket *socket = net_get_socket_from_port(dest_port);
    if (socket == NULL) {
        if (dest_port == DHCP_CLIENT_PORT) {
            net_dhcp_recieve((const struct dhcp_packet *) packet->payload, len - sizeof(struct udp_packet));
            return;
        }

        debug_log("UDP packet sent to unbound port: [ %u ]\n", dest_port);
        return;
    }

    const struct ip_v4_packet *ip_packet = container_of(packet, const struct ip_v4_packet, payload);
    struct socket_data *data =
        net_inet_create_socket_data(ip_packet, packet->source_port, (void *) packet->payload, len - sizeof(struct udp_packet));
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
