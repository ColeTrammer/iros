#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/icmp.h>
#include <kernel/net/interface.h>
#include <kernel/net/ip.h>
#include <kernel/net/route_cache.h>
#include <kernel/net/socket.h>
#include <kernel/net/tcp.h>
#include <kernel/net/udp.h>

int net_send_ip_v4(struct network_interface *interface, uint8_t protocol, struct ip_v4_address dest, const void *buf, size_t len) {
    if (interface->config_context.state != INITIALIZED) {
        debug_log("Can't send IP V4 packet; interface uninitialized: [ %s ]\n", interface->name);
        return -ENETDOWN;
    }

    struct route_cache_entry *route = net_find_next_hop_gateway(interface, dest);
    size_t total_size = sizeof(struct ip_v4_packet) + len;

    struct ip_v4_address d = dest;
    debug_log("Sending raw IPV4 to: [ %u.%u.%u.%u ]\n", d.addr[0], d.addr[1], d.addr[2], d.addr[3]);

    struct ip_v4_packet *ip_packet = net_create_ip_v4_packet(1, protocol, interface->address, dest, buf, len);
    int ret = interface->ops->send_ip_v4(interface, route, ip_packet, total_size);
    free(ip_packet);

    net_drop_route_cache_entry(route);
    return ret;
}

void net_ip_v4_recieve(const struct ip_v4_packet *packet, size_t len) {
    if (len < sizeof(struct ip_v4_packet)) {
        debug_log("IP V4 packet too small\n");
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
            net_tcp_recieve((const struct tcp_packet *) packet->payload, len - sizeof(struct ip_v4_packet));
            return;
        }
        default: {
            break;
        }
    }

    debug_log("Ignored packet\n");
}

struct ip_v4_packet *net_create_ip_v4_packet(uint16_t ident, uint8_t protocol, struct ip_v4_address source, struct ip_v4_address dest,
                                             const void *payload, uint16_t payload_length) {
    struct ip_v4_packet *packet = malloc(sizeof(struct ip_v4_packet) + payload_length);
    net_init_ip_v4_packet(packet, ident, protocol, source, dest, payload, payload_length);
    return packet;
}

void net_init_ip_v4_packet(struct ip_v4_packet *packet, uint16_t ident, uint8_t protocol, struct ip_v4_address source,
                           struct ip_v4_address dest, const void *payload, uint16_t payload_length) {
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
    if (payload) {
        memcpy(packet->payload, payload, payload_length);
    }
}
