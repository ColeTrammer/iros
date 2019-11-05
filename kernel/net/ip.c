#include <arpa/inet.h>
#include <assert.h>
#include <stddef.h>

#include <kernel/net/ip.h>

uint16_t net_ip_v4_compute_checksum(void *packet, size_t num_bytes) {
    uint16_t *raw_data = (uint16_t*) packet;
    uint32_t sum = 0;

    // Sum everything 16 bits at a time
    for (size_t i = 0; i < num_bytes / sizeof(uint16_t); i++) {
        // Prevent overflow
        if (sum & 0x80000000) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        sum += ntohs(raw_data[i]);
    }

    // 1's complement the carry
    while (sum & ~0xFFFF) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // Invert the sum for storage
    return (uint16_t) (~sum & 0xFFFF);
}

void net_init_ip_v4_packet(struct ip_v4_packet *packet, uint16_t ident, uint8_t protocol, struct ip_v4_address source, struct ip_v4_address dest, uint16_t payload_length) {
    assert(packet);

    packet->version_and_ihl = (IP_V4_VERSION << 4) | IP_V4_BYTES_TO_WORDS(sizeof(struct ip_v4_packet));
    packet->dscp_and_ecn = 0;
    packet->length = htons(sizeof(struct ip_v4_packet) + payload_length);
    packet->identification = htons(ident);
    packet->flags_and_fragment_offset = htons(0);
    packet->ttl = 64;
    packet->protocol = protocol;
    packet->source = source;
    packet->destination = dest;
    packet->checksum = 0;
    packet->checksum = htons(net_ip_v4_compute_checksum(packet, sizeof(struct ip_v4_packet)));
}