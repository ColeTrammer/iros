#ifndef _KERNEL_NET_TCP_H
#define _KERNEL_NET_TCP_H 1

#include <stdint.h>

#define TCP_FLAGS_FIN 1U
#define TCP_FLAGS_SYN 2U
#define TCP_FLAGS_RST 4U
#define TCP_FLAGS_PSH 8U
#define TCP_FLAGS_ACK 16U
#define TCP_FLAGS_URG 32U
#define TCP_FlAGS_ECE 64U
#define TCP_FLAGS_CWR 128U

#define TCP_DEFAULT_MSS     536
#define TCP_RTO_GRANULARITY 100

#define TCP_OPTION_END 0
#define TCP_OPTION_PAD 1
#define TCP_OPTION_MSS 2

struct destination_cache_entry;
struct networK_interface;
struct packet;
struct ring_buffer;
struct socket;

struct tcp_flags {
    uint8_t fin : 1;
    uint8_t syn : 1;
    uint8_t rst : 1;
    uint8_t psh : 1;
    uint8_t ack : 1;
    uint8_t urg : 1;
    uint8_t ece : 1;
    uint8_t cwr : 1;
} __attribute__((packed));

struct tcp_packet {
    uint16_t source_port;
    uint16_t dest_port;
    uint32_t sequence_number;
    uint32_t ack_number;

    uint8_t ns : 1;
    uint8_t zero : 3;
    uint8_t data_offset : 4;

    struct tcp_flags flags;

    uint16_t window_size;
    uint16_t check_sum;
    uint16_t urg_pointer;

    uint8_t options_and_payload[0];
} __attribute__((packed));

struct tcp_option {
    uint8_t type;
    uint8_t length;
    uint8_t data[0];
} __attribute__((packed));

struct tcp_option_mss {
    uint8_t type;
    uint8_t length;
    uint16_t mss;
} __attribute__((packed));

struct tcp_packet_options {
    uint16_t source_port;
    uint16_t dest_port;
    uint32_t sequence_number;
    uint32_t ack_number;
    struct tcp_flags tcp_flags;
    uint16_t window;
    uint16_t mss;
    uint32_t data_offset;
    uint16_t data_length;
    struct ring_buffer *data_rb;
    struct socket *socket;
    struct network_interface *interface;
    struct destination_cache_entry *destination;
};

int net_send_tcp_from_socket(struct socket *socket, uint32_t sequence_start, uint32_t sequence_end, bool send_rst, bool is_retransmission);
int net_send_tcp(struct ip_v4_address dest, struct tcp_packet_options *opts, struct timespec *send_time_ptr);
void net_tcp_recieve(struct packet *packet);
void net_init_tcp_packet(struct tcp_packet *packet, struct tcp_packet_options *opts);

void net_tcp_log(struct ip_v4_address source, struct ip_v4_address destination, struct packet *net_packet);

#endif /* _KERNEL_NET_TCP_H */
