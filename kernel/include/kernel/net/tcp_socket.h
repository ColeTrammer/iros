#ifndef _KERNEL_NET_TCP_SOCKET_H
#define _KERNEL_NET_TCP_SOCKET_H 1

#include <stdint.h>

#include <kernel/net/ip.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/ring_buffer.h>

struct timer;

enum tcp_state {
    TCP_CLOSED,
    TCP_LITSEN,
    TCP_SYN_SENT,
    TCP_SYN_RECIEVED,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT_1,
    TCP_FIN_WAIT_2,
    TCP_CLOSE_WAIT,
    TCP_CLOSING,
    TCP_LAST_ACK,
    TCP_TIME_WAIT,
};

struct tcp_connection_info {
    uint16_t source_port;
    uint16_t dest_port;
    struct ip_v4_address source_ip;
    struct ip_v4_address dest_ip;
} __attribute__((packed));

struct tcp_socket_mapping {
    struct tcp_connection_info key;
    struct hash_entry hash;
    struct socket *socket;
};

struct tcp_control_block {
    uint32_t send_unacknowledged;
    uint32_t send_next;
    uint32_t send_window;
    uint32_t send_wl1;
    uint32_t send_wl2;
    uint32_t recv_next;
    uint32_t recv_window;
    uint32_t segment_size;
    struct ring_buffer send_buffer;
    struct ring_buffer recv_buffer;
    struct timer *retransmission_timer;
    struct timespec retransmission_delay;
    enum tcp_state state;
    bool pending_syn : 1;
    bool pending_fin : 1;
    bool is_passive : 1;
};

struct socket *net_get_tcp_socket_by_connection_info(struct tcp_connection_info *info);

void net_free_tcp_control_block(struct socket *socket);

void init_tcp_sockets(void);

#endif /* _KERNEL_NET_TCP_SOCKET_H */
