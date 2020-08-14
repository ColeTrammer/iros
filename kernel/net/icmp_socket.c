#include <errno.h>

#include <kernel/net/icmp_socket.h>
#include <kernel/net/inet_socket.h>
#include <kernel/net/socket_syscalls.h>

static int net_icmp_socket(int domain, int type, int protocol);
static ssize_t net_icmp_sendto(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr *addr,
                               socklen_t addrlen);

static struct socket_ops icmp_ops = {
    .connect = net_inet_connect,
    .getpeername = net_inet_getpeername,
    .getsockopt = net_generic_getsockopt,
    .setsockopt = net_generic_setsockopt,
    .recvfrom = net_generic_recieve_from,
    .sendto = net_icmp_sendto,
};

static struct socket_protocol icmp_protocol = {
    .domain = AF_INET,
    .type = SOCK_RAW,
    .protocol = IPPROTO_ICMP,
    .is_default_protocol = false,
    .name = "IPv4 ICMP",
    .create_socket = net_icmp_socket,
};

static int net_icmp_socket(int domain, int type, int protocol) {
    int fd;
    net_create_socket_fd(domain, type, protocol, &icmp_ops, &fd, NULL);
    return fd;
}

static ssize_t net_icmp_sendto(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr *addr,
                               socklen_t addrlen) {
    (void) flags;

    ssize_t ret = 0;
    mutex_lock(&socket->lock);
    if (!addr) {
        if (!socket->has_peer_address) {
            ret = -EDESTADDRREQ;
            goto done;
        }
        addr = (struct sockaddr *) &socket->peer_address;
    } else if (addrlen < sizeof(struct sockaddr_in)) {
        ret = -EINVAL;
        goto done;
    }

    struct ip_v4_address dest_ip = IP_V4_FROM_SOCKADDR(addr);
    struct network_interface *interface = net_get_interface_for_ip(dest_ip);
    assert(interface);
    ret = net_send_ip_v4(interface, IP_V4_PROTOCOL_ICMP, dest_ip, buf, len);

done:
    mutex_unlock(&socket->lock);
    return ret;
}

void init_icmp_sockets(void) {
    net_register_protocol(&icmp_protocol);
}
