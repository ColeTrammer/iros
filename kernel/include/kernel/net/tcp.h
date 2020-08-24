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

struct ring_buffer;

union tcp_flags {
    struct {
        uint8_t fin : 1;
        uint8_t syn : 1;
        uint8_t rst : 1;
        uint8_t psh : 1;
        uint8_t ack : 1;
        uint8_t urg : 1;
        uint8_t ece : 1;
        uint8_t cwr : 1;
    } bits;
    uint8_t raw_flags;
} __attribute__((packed));

struct tcp_packet {
    uint16_t source_port;
    uint16_t dest_port;
    uint32_t sequence_number;
    uint32_t ack_number;

    uint8_t ns : 1;
    uint8_t zero : 3;
    uint8_t data_offset : 4;

    union tcp_flags flags;

    uint16_t window_size;
    uint16_t check_sum;
    uint16_t urg_pointer;

    uint8_t options_and_payload[0];
} __attribute__((packed));

int net_send_tcp_from_socket(struct socket *socket, uint32_t sequence_start, uint32_t sequence_end);
int net_send_tcp(struct ip_v4_address dest, uint16_t source_port, uint16_t dest_port, uint32_t sequence_number, uint32_t ack_num,
                 uint16_t window, union tcp_flags flags, uint16_t len, uint32_t offset, struct ring_buffer *rb);
void net_tcp_recieve(const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet, size_t len);
void net_init_tcp_packet(struct tcp_packet *packet, uint16_t source_port, uint16_t dest_port, uint32_t sequence, uint32_t ack_num,
                         union tcp_flags flags, uint16_t win_size, uint16_t payload_length, uint32_t offset, struct ring_buffer *rb);

void net_tcp_log(const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet);

#endif /* _KERNEL_NET_TCP_H */
