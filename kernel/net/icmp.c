#include <arpa/inet.h>
#include <assert.h>
#include <netinet/ip_icmp.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/icmp.h>
#include <kernel/net/inet_socket.h>
#include <kernel/net/ip.h>
#include <kernel/net/socket.h>
#include <kernel/util/macros.h>

static void icmp_for_each(struct hash_entry *_socket, void *_packet) {
    struct socket *socket = hash_table_entry(_socket, struct socket);
    struct ip_v4_packet *packet = _packet;
    if (socket->protocol == IPPROTO_ICMP) {
        size_t data_len = packet->length - sizeof(struct ip_v4_packet);
        struct socket_data *data = net_inet_create_socket_data(packet, 0, (void *) packet->payload, data_len);
        net_send_to_socket(socket, data);
    }
}

void net_icmp_recieve(const struct icmp_packet *packet, size_t len) {
    if (len < sizeof(struct icmp_packet)) {
        debug_log("ICMP packet too small\n");
        return;
    }

    if (packet->type == ICMP_TYPE_ECHO_REPLY) {
        net_for_each_socket(icmp_for_each, container_of(packet, struct ip_v4_packet, payload));
        return;
    }

    assert(packet->type == ICMP_TYPE_ECHO_REQUEST);

    const struct ip_v4_packet *ip_packet = container_of(packet, const struct ip_v4_packet, payload);
    size_t to_send_length = ip_packet->length - sizeof(struct ip_v4_packet);

    struct icmp_packet *to_send = malloc(to_send_length);
    net_init_icmp_packet(to_send, ICMP_TYPE_ECHO_REPLY, ntohs(packet->identifier), ntohs(packet->sequence_number), (void *) packet->payload,
                         to_send_length - sizeof(struct icmp_packet));

    net_send_ip_v4(net_get_interface_for_ip(ip_packet->source), IP_V4_PROTOCOL_ICMP, ip_packet->source, to_send, to_send_length);
    free(to_send);
}

void net_init_icmp_packet(struct icmp_packet *packet, uint8_t type, uint16_t identifier, uint16_t sequence, void *payload,
                          uint16_t payload_size) {
    assert(packet);

    packet->type = type;
    packet->code = 0;
    packet->identifier = htons(identifier);
    packet->sequence_number = htons(sequence);
    memcpy(packet->payload, payload, payload_size);

    packet->checksum = 0;
    packet->checksum = htons(in_compute_checksum(packet, sizeof(struct icmp_packet) + payload_size));
}
