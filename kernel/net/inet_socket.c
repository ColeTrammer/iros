#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/net/ethernet.h>
#include <kernel/net/inet_socket.h>
#include <kernel/net/interface.h>
#include <kernel/net/ip.h>
#include <kernel/net/port.h>

int net_inet_socket(int domain, int type, int protocol) {
    assert(domain == AF_INET);

    if (type == SOCK_STREAM || (protocol != IPPROTO_ICMP && protocol != IPPROTO_UDP)) {
        return -EPROTONOSUPPORT;
    }

    int fd;
    struct socket *socket = net_create_socket(domain, type, protocol, &fd);
    (void) socket;

    return fd;
}

ssize_t net_inet_sendto(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr_in *dest, socklen_t addrlen) {
    (void) flags;

    assert(socket);
    assert((socket->type & SOCK_TYPE_MASK) == SOCK_RAW);

    if (dest->sin_family != AF_INET || addrlen < sizeof(struct sockaddr_in)) {
        return -EINVAL;
    }

    struct network_interface *interface = net_get_interface_for_ip(ip_v4_from_uint(dest->sin_addr.s_addr));
    assert(interface);

    if ((socket->type & SOCK_TYPE_MASK) == SOCK_RAW) {
        size_t total_size = sizeof(struct ethernet_packet) + sizeof(struct ip_v4_packet) + len;

        struct ethernet_packet *packet = net_create_ethernet_packet(
            net_get_mac_from_ip_v4(interface->broadcast)->mac,
            interface->ops->get_mac_address(interface),
            ETHERNET_TYPE_IPV4,
            total_size - sizeof(struct ethernet_packet)
        );

        struct ip_v4_address d = ip_v4_from_uint(dest->sin_addr.s_addr);
        debug_log("Sending to: [ %lu, %u.%u.%u.%u ]\n", socket->id, d.addr[0], d.addr[1], d.addr[2], d.addr[3]);

        struct ip_v4_packet *ip_packet = (struct ip_v4_packet*) packet->payload; 
        net_init_ip_v4_packet(ip_packet, socket->id, socket->protocol, interface->address, ip_v4_from_uint(dest->sin_addr.s_addr), len);
        memcpy(ip_packet->payload, buf, len);

        ssize_t ret = interface->ops->send(interface, packet, total_size);

        free(packet);
        return ret <= 0 ? ret : ret - (ssize_t) sizeof(struct ethernet_packet) - (ssize_t) sizeof(struct ip_v4_packet);
    }

    assert(socket->type == SOCK_DGRAM);
    return -EAFNOSUPPORT;
}

ssize_t net_inet_recvfrom(struct socket *socket, void *buf, size_t len, int flags, struct sockaddr_in *source, socklen_t *addrlen) {
    (void) flags;
    (void) source;
    (void) addrlen;

    return net_generic_recieve(socket, buf, len);
}