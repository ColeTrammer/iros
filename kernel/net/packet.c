#include <stdlib.h>
#include <string.h>

#include <kernel/net/destination_cache.h>
#include <kernel/net/packet.h>
#include <kernel/net/socket.h>

void net_packet_write_headers(void *buffer, struct packet *packet, uint32_t start_header) {
    size_t offset = 0;
    for (uint32_t i = 0; i < packet->header_count; i++) {
        struct packet_header *header = &packet->headers[i];
        if (i >= start_header) {
            memcpy(buffer + offset, header->raw_header, header->length);
        }
        offset += header->length;
    }
}

void net_free_packet(struct packet *packet) {
    if (packet->socket) {
        net_drop_socket(packet->socket);
        packet->socket = NULL;
    }

    if (packet->destination) {
        net_drop_destination_cache_entry(packet->destination);
        packet->destination = NULL;
    }

    for (uint32_t i = 0; i < packet->header_count; i++) {
        if (packet->headers[i].flags & PHF_DYNAMICALLY_ALLOCATED) {
            free(packet->headers[i].raw_header);
        }
    }

    free(packet);
}

struct packet *net_create_packet(struct network_interface *interface, struct socket *socket, struct destination_cache_entry *destination,
                                 size_t inline_data_size) {
    struct packet *packet = malloc(sizeof(struct packet) + inline_data_size);
    packet->interface = interface;
    packet->socket = socket ? net_bump_socket(socket) : NULL;
    packet->destination = destination ? net_bump_destination_cache_entry(destination) : NULL;
    packet->header_count = packet->total_length = 0;
    memset(packet->headers, 0, sizeof(packet->headers));
    return packet;
}

const char *net_packet_header_type_to_string(enum packet_header_type type) {
    switch (type) {
        case PH_ETHERNET:
            return "Ethernet";
        case PH_ARP:
            return "ARP";
        case PH_IP_V4:
            return "IP v4";
        case PH_ICMP:
            return "ICMP";
        case PH_UDP:
            return "UDP";
        case PH_TCP:
            return "TCP";
        case PH_RAW_DATA:
            return "Raw Data";
        default:
            return "Invalid";
    }
}

void net_packet_log(const struct packet *packet) {
    debug_log("Packet: [ %u, %u ]\n", packet->header_count, packet->total_length);
    for (uint32_t i = 0; i < packet->header_count; i++) {
        const struct packet_header *header = &packet->headers[i];
        if (!(header->flags & PHF_INITIALIZED)) {
            debug_log("Header not initialized: [ %u ]\n", i);
            continue;
        }

        debug_log("Header: [ %u, %p, %6u, %#.2X, %s ]\n", i, header->raw_header, header->length, header->flags,
                  net_packet_header_type_to_string(header->type));
    }
}
