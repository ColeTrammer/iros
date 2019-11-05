#include <arpa/inet.h>
#include <assert.h>
#include <string.h>

#include <kernel/net/icmp.h>
#include <kernel/net/ip.h>

void net_init_icmp_packet(struct icmp_packet *packet, uint8_t type, uint16_t identifier, uint16_t sequence, void *payload, uint16_t payload_size) {
    assert(packet);

    packet->type = type;
    packet->code = 0;
    packet->identifier = htons(identifier);
    packet->sequence_number = htons(sequence);
    memcpy(packet->payload, payload, payload_size);

    packet->checksum = 0;
    packet->checksum = htons(net_ip_v4_compute_checksum(packet, sizeof(struct icmp_packet) + payload_size));
}