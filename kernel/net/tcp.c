#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/interface.h>
#include <kernel/net/ip.h>
#include <kernel/net/tcp.h>

ssize_t net_send_tcp(struct network_interface *interface, struct ip_v4_address dest, uint16_t source_port, uint16_t dest_port, union tcp_flags flags, uint16_t len, const void *payload) {
    size_t total_length = sizeof(struct ethernet_packet) + sizeof(struct ip_v4_packet) + sizeof(struct tcp_packet) + len;

    struct ethernet_packet *packet = net_create_ethernet_packet(net_get_mac_from_ip_v4(interface->broadcast)->mac, interface->ops->get_mac_address(interface),
        ETHERNET_TYPE_IPV4, total_length - sizeof(struct ethernet_packet));

    struct ip_v4_packet *ip_packet = (struct ip_v4_packet*) packet->payload;
    net_init_ip_v4_packet(ip_packet, 1, IP_V4_PROTOCOL_TCP, interface->address, dest, total_length - sizeof(struct ethernet_packet) - sizeof(struct ip_v4_packet));

    struct tcp_packet *tcp_packet = (struct tcp_packet*) ip_packet->payload;
    net_init_tcp_packet(tcp_packet, source_port, dest_port, 0, 0, flags, 0, len, payload);

    struct ip_v4_pseudo_header header = {
        interface->address, dest, 0, IP_V4_PROTOCOL_TCP, htons(len + sizeof(struct tcp_packet))
    };

    tcp_packet->check_sum = ntohs(in_compute_checksum_with_start(tcp_packet, sizeof(struct tcp_packet) + len,
        in_compute_checksum(&header, sizeof(struct ip_v4_pseudo_header))));

    debug_log("Sending TCP packet\n");
    net_tcp_log(tcp_packet);

    ssize_t ret = interface->ops->send(interface, packet, total_length);

    free(packet);
    return ret < 0 ? ret : ret - (ssize_t) sizeof(struct ethernet_packet) - (ssize_t) sizeof(struct ip_v4_packet);
}

void net_tcp_recieve(const struct tcp_packet *packet, size_t len) {
    if (len < sizeof(struct tcp_packet)) {
        debug_log("TCP Packet to small\n");
        return;
    }

    net_tcp_log(packet);
}

void net_init_tcp_packet(struct tcp_packet *packet, uint16_t source_port, uint16_t dest_port, uint32_t sequence, uint32_t ack_num, union tcp_flags flags, uint16_t win_size, uint16_t payload_length, const void *payload) {
    packet->source_port = htons(source_port);
    packet->dest_port = htons(dest_port);
    packet->sequence_number = htonl(sequence);
    packet->ack_number = htonl(ack_num);
    packet->ns = 0;
    packet->zero = 0;
    packet->data_offset = sizeof(struct tcp_packet) / sizeof(uint32_t);
    packet->flags = flags;
    packet->window_size = htons(win_size);
    packet->check_sum = 0;
    packet->urg_pointer = 0;

    if (payload_length > 0 && payload != NULL) {
        memcpy(packet->options_and_payload, payload, payload_length);
    }
}

void net_tcp_log(const struct tcp_packet *packet) {
    const struct ip_v4_packet *ip_packet = ((const struct ip_v4_packet*) packet) - 1;

    debug_log("TCP Packet:\n"
              "               Source Port  [ %15u ]   Dest Port [ %15u ]\n"
              "               Sequence     [ %15u ]   Ack Num   [ %15u ]\n"
              "               Win Size     [ %15u ]   Urg Point [ %15u ]\n"
              "               Source IP    [ %03u.%03u.%03u.%03u ]   Dest IP   [ %03u.%03u.%03u.%03u ]\n"   
              "               Data Len  [ %15lu ]\n"
              "               Flags        [ FIN=%u SYN=%u RST=%u PSH=%u ACK=%u URG=%u ECE=%u CWR=%u ]\n",
              ntohs(packet->source_port), ntohs(packet->dest_port), ntohl(packet->sequence_number), ntohl(packet->ack_number),
              ntohs(packet->window_size), ntohs(packet->urg_pointer),
              ip_packet->source.addr[0], ip_packet->source.addr[1], ip_packet->source.addr[2], ip_packet->source.addr[3],
              ip_packet->destination.addr[0], ip_packet->destination.addr[1], ip_packet->destination.addr[2], ip_packet->destination.addr[3],
              ntohs(ip_packet->length) - sizeof(struct ip_v4_packet) - sizeof(uint32_t) * packet->data_offset,
              packet->flags.bits.fin, packet->flags.bits.syn, packet->flags.bits.rst, packet->flags.bits.psh,
              packet->flags.bits.ack, packet->flags.bits.urg, packet->flags.bits.ece, packet->flags.bits.cwr
    );
}