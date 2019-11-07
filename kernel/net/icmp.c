#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/icmp.h>
#include <kernel/net/ip.h>
#include <kernel/net/socket.h>

static void icmp_for_each(struct socket *socket, void *_packet) {
    struct ip_v4_packet *packet = _packet;
    if (socket->protocol == IPPROTO_ICMP) {
        size_t data_len = packet->length - sizeof(struct ip_v4_packet);
        struct socket_data *data = calloc(1, sizeof(struct socket_data) + data_len);
        data->len = data_len;
        memcpy(data->data, packet->payload, data_len);
        net_send_to_socket(socket, data);
    }
}

void net_icmp_recieve(struct icmp_packet *packet, size_t len) {
    if (len < sizeof(struct icmp_packet)) {
        debug_log("ICMP packet too small\n");
        return;
    }

    net_for_each_socket(icmp_for_each, packet);
}

void net_init_icmp_packet(struct icmp_packet *packet, uint8_t type, uint16_t identifier, uint16_t sequence, void *payload, uint16_t payload_size) {
    assert(packet);

    packet->type = type;
    packet->code = 0;
    packet->identifier = htons(identifier);
    packet->sequence_number = htons(sequence);
    memcpy(packet->payload, payload, payload_size);

    packet->checksum = 0;
    packet->checksum = htons(in_compute_checksum(packet, sizeof(struct icmp_packet) + payload_size));
}