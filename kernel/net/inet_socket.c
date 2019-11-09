#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/net/ethernet.h>
#include <kernel/net/inet_socket.h>
#include <kernel/net/interface.h>
#include <kernel/net/ip.h>
#include <kernel/net/port.h>
#include <kernel/net/socket.h>
#include <kernel/net/tcp.h>
#include <kernel/net/udp.h>

struct socket_data *net_inet_create_socket_data(const struct ip_v4_packet *packet, uint16_t port_network_ordered, const void *buf, size_t len) {
    struct socket_data *data = calloc(1, sizeof(struct socket_data) + len);
    assert(data);

    data->len = len;
    memcpy(data->data, buf, len);
    data->from.addrlen = sizeof(struct sockaddr_in);
    data->from.addr.in.sin_family = AF_INET;
    data->from.addr.in.sin_port = port_network_ordered;
    data->from.addr.in.sin_addr.s_addr = ip_v4_to_uint(packet->source);

    return data;
}

int net_inet_bind(struct socket *socket, const struct sockaddr_in *addr, socklen_t addrlen) {
    assert(socket);

    if (addr->sin_family != AF_INET || addrlen < sizeof(struct sockaddr_in)) {
        return -EINVAL;
    }

    struct inet_socket_data *data = calloc(1, sizeof(struct inet_socket_data));
    socket->private_data = data;

    if (addr->sin_port == 0) {
        int ret = net_bind_to_ephemeral_port(socket->id, &data->source_port);
        if (ret < 0) {
            return ret;
        }
    } else {
        // TODO: bind to a specified port
        return -EADDRINUSE;
    }

    socket->state = BOUND;
    return 0;
}

int net_inet_close(struct socket *socket) {
    assert(socket);

    struct inet_socket_data *data = socket->private_data;
    if (data) {
        net_unbind_port(data->source_port);
        free(data);
    }

    return 0;
}

int net_inet_connect(struct socket *socket, const struct sockaddr_in *addr, socklen_t addrlen) {
    assert(socket);

    if (addr->sin_family != AF_INET || addrlen < sizeof(struct sockaddr_in)) {
        return -EINVAL;
    }

    if (socket->state != BOUND) {
        struct sockaddr_in to_bind = { AF_INET, 0, { 0 }, { 0 } };
        int ret = net_inet_bind(socket, &to_bind, sizeof(struct sockaddr_in));
        if (ret < 0) {
            return ret;
        }
    }

    struct inet_socket_data *data = socket->private_data;
    struct ip_v4_address dest_ip = ip_v4_from_uint(addr->sin_addr.s_addr);
    struct network_interface *interface = net_get_interface_for_ip(dest_ip);

    net_send_tcp(interface, dest_ip, data->source_port, ntohs(addr->sin_port), (union tcp_flags) { .raw_flags=TCP_FLAGS_SYN }, 0, NULL);

    return -ENETDOWN;
}

int net_inet_socket(int domain, int type, int protocol) {
    assert(domain == AF_INET);

    if (protocol == 0 && type == SOCK_DGRAM) {
        protocol = IPPROTO_UDP;
    }

    if (protocol == 0 && type == SOCK_STREAM) {
        protocol = IPPROTO_TCP;
    }

    if (protocol != IPPROTO_ICMP && protocol != IPPROTO_UDP && protocol != IPPROTO_TCP) {
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
    assert((socket->type & SOCK_TYPE_MASK) == SOCK_RAW || (socket->type & SOCK_TYPE_MASK) == SOCK_DGRAM);

    if (dest->sin_family != AF_INET || addrlen < sizeof(struct sockaddr_in)) {
        return -EINVAL;
    }

    struct network_interface *interface = net_get_interface_for_ip(ip_v4_from_uint(dest->sin_addr.s_addr));
    assert(interface);

    if ((socket->type & SOCK_TYPE_MASK) == SOCK_RAW) {
        return net_send_ip_v4(interface, socket->protocol, ip_v4_from_uint(dest->sin_addr.s_addr), buf, len);
    }

    assert(socket->type == SOCK_DGRAM && socket->protocol == IPPROTO_UDP);
    if (socket->state != BOUND) {
        struct sockaddr_in to_bind = { AF_INET, 0, { 0 }, { 0 } };
        int ret = net_inet_bind(socket, &to_bind, sizeof(struct sockaddr_in));
        if (ret < 0) {
            return ret;
        }
    }

    if (dest == NULL) {
        return -EINVAL;
    }

    struct inet_socket_data *data = socket->private_data;
    struct ip_v4_address dest_ip = ip_v4_from_uint(dest->sin_addr.s_addr);
    return net_send_udp(interface, dest_ip, data->source_port, ntohs(dest->sin_port), len, buf);
}

ssize_t net_inet_recvfrom(struct socket *socket, void *buf, size_t len, int flags, struct sockaddr_in *source, socklen_t *addrlen) {
    (void) flags;

    return net_generic_recieve_from(socket, buf, len, (struct sockaddr*) source, addrlen);
}