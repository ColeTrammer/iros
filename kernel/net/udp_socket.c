#include <errno.h>

#include <kernel/net/inet_socket.h>
#include <kernel/net/port.h>
#include <kernel/net/socket_syscalls.h>
#include <kernel/net/udp.h>

static int net_udp_socket(int domain, int type, int protocol);
static ssize_t net_udp_sendto(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr *addr,
                              socklen_t addrlen);

static struct socket_ops udp_ops = {
    .bind = net_inet_bind,
    .close = net_inet_close,
    .connect = net_inet_connect,
    .getpeername = net_inet_getpeername,
    .recvfrom = net_generic_recieve_from,
    .sendto = net_udp_sendto,
};

static struct socket_protocol udp_protocol = {
    .domain = AF_INET,
    .type = SOCK_DGRAM,
    .protocol = IPPROTO_UDP,
    .is_default_protocol = true,
    .name = "IPv4 UDP",
    .create_socket = net_udp_socket,
};

static int net_udp_socket(int domain, int type, int protocol) {
    int fd;
    net_create_socket_fd(domain, type, protocol, &udp_ops, &fd, NULL);
    return fd;
}

static ssize_t net_udp_sendto(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr *addr,
                              socklen_t addrlen) {
    (void) flags;
    if (addrlen < sizeof(struct sockaddr_in)) {
        return -EINVAL;
    }

    ssize_t ret = 0;
    mutex_lock(&socket->lock);
    if (!addr) {
        if (!socket->has_peer_address) {
            ret = -EDESTADDRREQ;
            goto done;
        }
        addr = (struct sockaddr *) &socket->peer_address;
    }

    if (socket->state != BOUND) {
        // Bind the socket to an ephemeral port by passing sin_port = 0.
        struct sockaddr_in ephemeral_address = {
            .sin_family = AF_INET, .sin_port = 0, .sin_addr = { .s_addr = INADDR_ANY }, .sin_zero = { 0 }
        };
        ret = udp_ops.bind(socket, (struct sockaddr *) &ephemeral_address, sizeof(ephemeral_address));
        if (ret) {
            goto done;
        }
    }

    ret = net_send_udp_through_socket(socket, buf, len, addr);

done:
    mutex_unlock(&socket->lock);
    return ret;
}

void init_udp_sockets(void) {
    net_register_protocol(&udp_protocol);
}
