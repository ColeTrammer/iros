#include <arpa/inet.h>
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/icmp.h>
#include <kernel/net/ip.h>
#include <kernel/net/socket.h>

void net_ip_v4_recieve(struct ip_v4_packet *packet, size_t len) {
    if (len < sizeof(struct ip_v4_packet)) {
        debug_log("IP V4 packet too small\n");
        return;
    }

    switch (packet->protocol) {
        case IP_V4_PROTOCOL_ICMP: {
            net_icmp_recieve((struct icmp_packet*) packet->payload, len - sizeof(struct ip_v4_packet));
            return;
        }
        default: {
            break;
        }
    }

    debug_log("Ignored packet\n");
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
    packet->checksum = htons(in_compute_checksum(packet, sizeof(struct ip_v4_packet)));
}