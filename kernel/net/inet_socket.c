#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <kernel/hal/processor.h>
#include <kernel/hal/timer.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/icmp_socket.h>
#include <kernel/net/inet_socket.h>
#include <kernel/net/interface.h>
#include <kernel/net/ip.h>
#include <kernel/net/port.h>
#include <kernel/net/socket.h>
#include <kernel/net/socket_syscalls.h>
#include <kernel/net/tcp.h>
#include <kernel/net/tcp_socket.h>
#include <kernel/net/udp.h>
#include <kernel/net/udp_socket.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/hash_map.h>

struct socket_data *net_inet_create_socket_data(const struct ip_v4_packet *packet, uint16_t port_network_ordered, const void *buf,
                                                size_t len) {
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

int net_inet_bind(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen) {
    if (addrlen < sizeof(struct sockaddr_in)) {
        return -EINVAL;
    }

    uint16_t source_port = PORT_FROM_SOCKADDR(addr);
    if (source_port == 0) {
        int ret = net_bind_to_ephemeral_port(socket, &source_port);
        if (ret < 0) {
            return ret;
        }
    } else {
        int ret = net_bind_to_port(socket, source_port);
        if (ret < 0) {
            return ret;
        }
    }

    struct sockaddr_in host_address = {
        .sin_family = AF_INET, .sin_port = htons(source_port), .sin_addr = { ip_v4_to_uint(IP_V4_FROM_SOCKADDR(addr)) }, .sin_zero = { 0 }
    };
    net_set_host_address(socket, &host_address, sizeof(host_address));

    socket->state = BOUND;
    return 0;
}

int net_inet_close(struct socket *socket) {
    if (socket->has_host_address) {
        uint16_t port = PORT_FROM_SOCKADDR(&socket->host_address);
        if (net_get_socket_from_port(port) == socket) {
            net_unbind_port(port);
        }
    }
    return 0;
}

int net_inet_connect(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen) {
    if (addrlen < sizeof(struct sockaddr_in)) {
        return -EINVAL;
    }

    net_set_peer_address(socket, addr, addrlen);
    return 0;
}

int net_inet_getpeername(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen) {
    int ret = 0;

    mutex_lock(&socket->lock);
    if (socket->has_peer_address) {
        net_copy_sockaddr_to_user(&socket->peer_address, sizeof(struct sockaddr_in), addr, addrlen);
    } else {
        ret = -ENOTCONN;
    }
    mutex_unlock(&socket->lock);

    return ret;
}

int net_inet_getsockname(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen) {
    int ret = 0;

    mutex_lock(&socket->lock);
    if (socket->has_host_address) {
        net_copy_sockaddr_to_user(&socket->host_address, sizeof(struct sockaddr_in), addr, addrlen);
    } else {
        ret = -ENOTCONN;
    }
    mutex_unlock(&socket->lock);

    return ret;
}

ssize_t net_inet_sendto(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen) {
    (void) flags;

    int ret = 0;
    mutex_lock(&socket->lock);
    if (!addr) {
        if (!socket->has_peer_address) {
            ret = -EDESTADDRREQ;
            goto fail;
        }
        addr = (struct sockaddr *) &socket->peer_address;
    } else if (addrlen < sizeof(struct sockaddr_in)) {
        ret = -EINVAL;
        goto fail;
    }

    struct ip_v4_address dest_ip = IP_V4_FROM_SOCKADDR(addr);
    struct network_interface *interface = net_get_interface_for_socket(socket, dest_ip);
    mutex_unlock(&socket->lock);

    ret = net_send_ip_v4(socket, interface, socket->protocol, dest_ip, buf, len);
    return ret ? ret : (ssize_t) len;

fail:
    mutex_unlock(&socket->lock);
    return ret;
}
