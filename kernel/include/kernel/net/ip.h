#ifndef _KERNEL_NET_IP_H
#define _KERNEL_NET_IP_H 1

#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/net/interface.h>
#include <kernel/net/ip_address.h>

#define IP_V4_VERSION 4

#define IP_V4_BYTES_TO_WORDS(bytes) ((bytes) / sizeof(uint32_t))

#define IP_V4_PROTOCOL_ICMP 0x01
#define IP_V4_PROTOCOL_TCP  0x06
#define IP_V4_PROTOCOL_UDP  0x11

#define IP_V4_MAX_PACKET_LENGTH 65535U

enum packet_header_type;

struct packet;
struct socket;

struct ip_v4_packet {
    uint8_t ihl : 4;
    uint8_t version : 4;
    uint8_t ecn : 2;
    uint8_t dscp : 6;
    uint16_t length;
    uint16_t identification;
    uint8_t fragment_offset_high : 5;
    uint8_t more_fragments : 1;
    uint8_t dont_fragment : 1;
    uint8_t reserved_flag : 1;
    uint8_t fragment_offset_low;
#define IP_V4_FRAGMENT_OFFSET(packet) ((((packet)->fragment_offset_high << 8) + ((packet)->fragment_offset_low)) << 3)
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;

    struct ip_v4_address source;
    struct ip_v4_address destination;

    uint8_t payload[0];
} __attribute__((packed));

_Static_assert(sizeof(struct ip_v4_packet) == 20);

struct ip_v4_pseudo_header {
    struct ip_v4_address source;
    struct ip_v4_address dest;
    uint8_t zero;
    uint8_t protocol;
    uint16_t length;
} __attribute__((packed));

uint8_t net_packet_header_to_ip_v4_type(enum packet_header_type type);
enum packet_header_type net_inet_protocol_to_packet_header_type(uint8_t protocol);
int net_interface_route_ip_v4(struct network_interface *self, struct destination_cache_entry *destination, struct packet *packet);
int net_send_ip_v4(struct socket *socket, struct network_interface *interface, uint8_t protocol, struct ip_v4_address dest, const void *buf,
                   size_t len);
void net_ip_v4_recieve(struct packet *packet);

void net_init_ip_v4_packet(struct ip_v4_packet *packet, uint16_t ident, uint8_t protocol, struct ip_v4_address source,
                           struct ip_v4_address dest, const void *payload, uint16_t payload_length);
void net_ip_v4_log(const struct ip_v4_packet *packet);
void init_ip_v4(void);

#endif /* _KERNEL_NET_IP_H */
