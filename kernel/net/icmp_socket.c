#include <errno.h>

#include <kernel/net/icmp_socket.h>
#include <kernel/net/inet_socket.h>
#include <kernel/net/socket_syscalls.h>

static int net_icmp_socket(int domain, int type, int protocol);

static struct socket_ops icmp_ops = {
    .connect = net_inet_connect,
    .getpeername = net_inet_getpeername,
    .getsockopt = net_generic_getsockopt,
    .setsockopt = net_generic_setsockopt,
    .recvfrom = net_generic_recieve_from,
    .sendto = net_inet_sendto,
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

void init_icmp_sockets(void) {
    net_register_protocol(&icmp_protocol);
}
