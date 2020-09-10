#ifndef _KERNEL_NET_PACKET_H
#define _KERNEL_NET_PACKET_H 1

#include <stdint.h>
#include <kernel/util/list.h>

#define NET_PACKET_MAX_HEADERS 5

struct destination_cache_entry;
struct network_interface;
struct socket;

enum packet_header_type {
    PH_ETHERNET,
    PH_ARP,
    PH_IP_V4,
    PH_ICMP,
    PH_UDP,
    PH_TCP,
    PH_RAW_DATA,
};

struct packet_header {
    enum packet_header_type type;
#define PHF_INITIALIZED           1
#define PHF_DYNAMICALLY_ALLOCATED 2
    uint8_t flags;
    uint16_t length;
    void *raw_header;
};

struct packet {
    struct list_node queue;
    struct network_interface *interface;
    struct socket *socket;
    struct destination_cache_entry *destination;
    uint32_t header_count;
    uint32_t total_length;
    struct packet_header headers[NET_PACKET_MAX_HEADERS];
    uint8_t inline_data[0];
};

void net_packet_write_headers(void *buffer, struct packet *packet, uint32_t start_header);

void net_free_packet(struct packet *packet);
struct packet *net_create_packet(struct network_interface *interface, struct socket *socket, struct destination_cache_entry *destination,
                                 size_t inline_data_size);
void net_packet_log(const struct packet *packet);
const char *net_packet_header_type_to_string(enum packet_header_type type);

static inline uint32_t net_packet_header_index(struct packet *packet, struct packet_header *header) {
    return header - packet->headers;
}

static inline struct packet_header *net_packet_inner_header(struct packet *packet) {
    return &packet->headers[packet->header_count - 1];
}

static inline struct packet_header *net_packet_innermost_header_of_type(struct packet *packet, enum packet_header_type type) {
    for (uint32_t i = packet->header_count; i > 0; i--) {
        struct packet_header *header = &packet->headers[i - 1];
        if (header->type == type) {
            return header;
        }
    }
    return NULL;
}

static inline struct packet_header *net_packet_outer_header(struct packet *packet) {
    for (uint32_t i = 0; i < packet->header_count; i++) {
        struct packet_header *header = &packet->headers[i];
        if (header->flags & PHF_INITIALIZED) {
            return header;
        }
    }
    return NULL;
}

static inline struct packet_header *net_packet_add_header(struct packet *packet, size_t inner_header_length) {
    struct packet_header *inner_header = net_packet_inner_header(packet);
    struct packet_header *next_header = &packet->headers[packet->header_count++];
    next_header->flags |= PHF_INITIALIZED;
    next_header->length = inner_header->length - inner_header_length;
    next_header->raw_header = inner_header->raw_header + inner_header_length;
    inner_header->length = inner_header_length;
    return next_header;
}

static inline struct packet_header *net_init_packet_header(struct packet *packet, uint32_t header_index, enum packet_header_type type,
                                                           void *raw_header, uint16_t header_length) {
    struct packet_header *header = &packet->headers[header_index];
    header->raw_header = raw_header;
    header->flags |= PHF_INITIALIZED;
    header->type = type;
    header->length = header_length;
    packet->total_length += header_length;
    return header;
}

#endif /* _KERNEL_NET_PACKET_H */
