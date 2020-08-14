#ifndef _KERNEL_NET_TCP_SOCKET_H
#define _KERNEL_NET_TCP_SOCKET_H 1

#include <stdint.h>

#include <kernel/net/ip.h>

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

struct socket *net_get_tcp_socket_by_ip_v4_and_port(struct ip_v4_and_port tuple);
struct socket *net_get_tcp_socket_server_by_ip_v4_and_port(struct ip_v4_and_port tuple);

void init_tcp_sockets(void);

#endif /* _KERNEL_NET_TCP_SOCKET_H */
