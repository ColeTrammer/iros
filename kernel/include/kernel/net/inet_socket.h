#ifndef _KERNEL_NET_INET_SOCKET_H
#define _KERNEL_NET_INET_SOCKET_H 1

#include <netinet/in.h>

#include <kernel/net/ip.h>
#include <kernel/net/ip_address.h>
#include <kernel/net/socket.h>
#include <kernel/util/hash_map.h>

struct ip_v4_and_port {
    uint16_t port;
    struct ip_v4_address ip;
} __attribute__((packed));

struct tcp_socket_mapping {
    struct ip_v4_and_port key;
    unsigned long socket_id;
};

struct tcp_control_block {
    uint32_t current_sequence_num;
    uint32_t current_ack_num;
    bool should_send_ack;
    struct hash_map *sent_packets;
};

struct tcp_packet_data {
    uint32_t sequence_number;
    size_t data_len;
    uint8_t data[0];
};

struct inet_socket_data {
    uint16_t source_port;
    uint16_t dest_port;
    struct ip_v4_address source_ip;
    struct ip_v4_address dest_ip;
    struct tcp_control_block *tcb;
};

struct socket_data *net_inet_create_socket_data(const struct ip_v4_packet *packet, uint16_t port_network_ordered, const void *buf,
                                                size_t len);

int net_inet_accept(struct socket *socket, struct sockaddr_in *addr, socklen_t *addrlen, int flags);
int net_inet_bind(struct socket *socket, const struct sockaddr_in *addr, socklen_t addrlen);
int net_inet_close(struct socket *socket);
int net_inet_connect(struct socket *socket, const struct sockaddr_in *addr, socklen_t addrlen);
int net_inet_listen(struct socket *socket);
int net_inet_socket(int domain, int type, int protocol);

ssize_t net_inet_sendto(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr_in *dest, socklen_t addrlen);
ssize_t net_inet_recvfrom(struct socket *socket, void *buf, size_t len, int flags, struct sockaddr_in *source, socklen_t *addrlen);

struct socket *net_get_tcp_socket_by_ip_v4_and_port(struct ip_v4_and_port tuple);
struct socket *net_get_tcp_socket_server_by_ip_v4_and_port(struct ip_v4_and_port tuple);

void init_inet_sockets();

#endif /* _KERNEL_NET_INET_SOCKET_H */