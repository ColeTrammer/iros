#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/destination_cache.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/icmp.h>
#include <kernel/net/interface.h>
#include <kernel/net/ip.h>
#include <kernel/net/network_task.h>
#include <kernel/net/socket.h>
#include <kernel/net/tcp.h>
#include <kernel/net/udp.h>

// #define IP_V4_DEBUG

int net_send_ip_v4(struct socket *socket, struct network_interface *interface, uint8_t protocol, struct ip_v4_address dest, const void *buf,
                   size_t len) {
    if (interface->config_context.state != INITIALIZED) {
        debug_log("Can't send IP V4 packet; interface uninitialized: [ %s ]\n", interface->name);
        return -ENETDOWN;
    }

    struct destination_cache_entry *destination = net_lookup_destination(interface, dest);

    struct ip_v4_address d = dest;
    debug_log("Sending raw IPV4 to: [ %u.%u.%u.%u ]\n", d.addr[0], d.addr[1], d.addr[2], d.addr[3]);

    struct network_data *ip_packet =
        net_create_ip_v4_packet(interface, socket, destination->next_packet_id++, protocol, interface->address, dest, buf, len);
    int ret = interface->ops->send_ip_v4(interface, destination, ip_packet);

    net_drop_destination_cache_entry(destination);
    return ret;
}

void net_ip_v4_recieve(const struct ip_v4_packet *packet, size_t len) {
    if (len < sizeof(struct ip_v4_packet)) {
        debug_log("IP V4 packet too small\n");
        return;
    }

#ifdef IP_V4_DEBUG
    net_ip_v4_log(packet);
#endif /* IP_V4_DEBUG */

    if (packet->more_fragments || packet->fragment_offset > 0) {
        debug_log("IP V4 packet is fragmented\n");
        return;
    }

    switch (packet->protocol) {
        case IP_V4_PROTOCOL_ICMP: {
            net_icmp_recieve((const struct icmp_packet *) packet->payload, len - sizeof(struct ip_v4_packet));
            return;
        }
        case IP_V4_PROTOCOL_UDP: {
            net_udp_recieve((const struct udp_packet *) packet->payload, len - sizeof(struct ip_v4_packet));
            return;
        }
        case IP_V4_PROTOCOL_TCP: {
            net_tcp_recieve(packet, (const struct tcp_packet *) packet->payload, len - sizeof(struct ip_v4_packet));
            return;
        }
        default: {
            break;
        }
    }

    debug_log("Ignored packet\n");
}

struct network_data *net_create_ip_v4_packet(struct network_interface *interface, struct socket *socket, uint16_t ident, uint8_t protocol,
                                             struct ip_v4_address source, struct ip_v4_address dest, const void *payload,
                                             uint16_t payload_length) {
    struct network_data *data = malloc(sizeof(struct network_data) + sizeof(struct ip_v4_packet) + payload_length);
    data->interface = interface;
    data->socket = socket ? net_bump_socket(socket) : NULL;
    data->type = NETWORK_DATA_IP_V4;
    data->len = sizeof(struct ip_v4_packet) + payload_length;
    data->ip_v4_packet = (struct ip_v4_packet *) (data + 1);
    net_init_ip_v4_packet(data->ip_v4_packet, ident, protocol, source, dest, payload, payload_length);
    return data;
}

void net_init_ip_v4_packet(struct ip_v4_packet *packet, uint16_t ident, uint8_t protocol, struct ip_v4_address source,
                           struct ip_v4_address dest, const void *payload, uint16_t payload_length) {
    packet->version = IP_V4_VERSION;
    packet->ihl = IP_V4_BYTES_TO_WORDS(sizeof(struct ip_v4_packet));
    packet->dscp = 0;
    packet->ecn = 0;
    packet->length = htons(sizeof(struct ip_v4_packet) + payload_length);
    packet->identification = htons(ident);
    packet->reserved_flag = 0;
    packet->dont_fragment = 0;
    packet->more_fragments = 0;
    packet->fragment_offset = htons(0);
    packet->ttl = 64;
    packet->protocol = protocol;
    packet->source = source;
    packet->destination = dest;
    packet->checksum = 0;
    packet->checksum = htons(in_compute_checksum(packet, sizeof(struct ip_v4_packet)));
    if (payload) {
        memcpy(packet->payload, payload, payload_length);
    }
}

void net_ip_v4_log(const struct ip_v4_packet *ip_packet) {
    debug_log("IP v4 Packet:\n"
              "               Header Len   [ %15u ]   Version   [ %15u ]\n"
              "               DSCP         [ %15u ]   ECN       [ %15u ]\n"
              "               Length       [ %15u ]   ID        [ %15u ]\n"
              "               Flags        [ DF=%u MF=%u       ]   Frag Off  [ %15u ]\n"
              "               TTL          [ %15u ]   Protocol  [ %15u ]\n"
              "               Source IP    [ %03u.%03u.%03u.%03u ]   Dest IP   [ %03u.%03u.%03u.%03u ]\n"
              "               Data Len     [ %15u ]   Data off  [ %15u ]\n",
              ip_packet->ihl, ip_packet->version, ip_packet->dscp, ip_packet->ecn, ntohs(ip_packet->length),
              ntohs(ip_packet->identification), ip_packet->dont_fragment, ip_packet->more_fragments, ntohs(ip_packet->fragment_offset),
              ip_packet->ttl, ip_packet->protocol, ip_packet->source.addr[0], ip_packet->source.addr[1], ip_packet->source.addr[2],
              ip_packet->source.addr[3], ip_packet->destination.addr[0], ip_packet->destination.addr[1], ip_packet->destination.addr[2],
              ip_packet->destination.addr[3], ntohs(ip_packet->length) - ip_packet->ihl * (uint32_t) sizeof(uint32_t),
              ip_packet->ihl * (uint32_t) sizeof(uint32_t));
}
