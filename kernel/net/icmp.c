#include <arpa/inet.h>
#include <assert.h>
#include <netinet/ip_icmp.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/icmp.h>
#include <kernel/net/inet_socket.h>
#include <kernel/net/ip.h>
#include <kernel/net/packet.h>
#include <kernel/net/socket.h>
#include <kernel/util/checksum.h>
#include <kernel/util/macros.h>

void net_icmp_recieve(struct packet *net_packet) {
    struct packet_header *icmp_header = net_packet_inner_header(net_packet);
    if (icmp_header->length < sizeof(struct icmp_packet)) {
        debug_log("ICMP packet too small\n");
        return;
    }

    struct packet_header *ip_header = net_packet_innermost_header_of_type(net_packet, PH_IP_V4);
    assert(ip_header);

    struct icmp_packet *packet = icmp_header->raw_header;
    struct ip_v4_packet *ip_packet = ip_header->raw_header;

    if (packet->type == ICMP_TYPE_ECHO_REPLY) {
        net_for_each_socket(socket) {
            if (socket->protocol == IPPROTO_ICMP) {
                size_t data_len = icmp_header->length;
                struct socket_data *data = net_inet_create_socket_data(ip_packet, 0, packet, data_len);
                net_send_to_socket(socket, data);
            }
        }
        return;
    }

    if (packet->type != ICMP_TYPE_ECHO_REQUEST) {
        debug_log("Unkown ICMP type: [ %u ]\n", packet->type);
        return;
    }

    size_t to_send_length = icmp_header->length;
    struct icmp_packet *to_send = malloc(to_send_length);
    net_init_icmp_packet(to_send, ICMP_TYPE_ECHO_REPLY, ntohs(packet->identifier), ntohs(packet->sequence_number), (void *) packet->payload,
                         to_send_length - sizeof(struct icmp_packet));

    net_send_ip_v4(NULL, net_get_interface_for_ip(ip_packet->source), IP_V4_PROTOCOL_ICMP, ip_packet->source, to_send, to_send_length);
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
    packet->checksum = htons(compute_internet_checksum(packet, sizeof(struct icmp_packet) + payload_size));
}
