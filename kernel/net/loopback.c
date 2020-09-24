#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/net/destination_cache.h>
#include <kernel/net/interface.h>
#include <kernel/net/ip.h>
#include <kernel/net/loopback.h>
#include <kernel/net/packet.h>
#include <kernel/util/init.h>

static int loop_route_ip_v4(struct network_interface *interface, struct packet *packet) {
    struct packet_header *outer_header = net_packet_outer_header(packet);
    struct destination_cache_entry *destination = packet->destination;

    struct ip_v4_packet *ip_packet = malloc(sizeof(struct ip_v4_packet));
    net_init_ip_v4_packet(ip_packet, destination ? destination->next_packet_id++ : 0, net_packet_header_to_ip_v4_type(outer_header->type),
                          packet->interface->address, destination ? destination->destination_path.dest_ip_address : IP_V4_BROADCAST, NULL,
                          packet->total_length);

    struct packet_header *ip_header =
        net_init_packet_header(packet, net_packet_header_index(packet, outer_header) - 1, PH_IP_V4, ip_packet, sizeof(struct ip_v4_packet));
    ip_header->flags |= PHF_DYNAMICALLY_ALLOCATED;

    net_recieve_packet(interface, packet);
    return 0;
}

static struct network_interface_ops ops = { .route_ip_v4 = loop_route_ip_v4 };

static void init_loopback() {
    net_create_network_interface("lo", NETWORK_INTERFACE_LOOPBACK, LINK_LAYER_ADDRESS_NONE, &ops, NULL);
}
INIT_FUNCTION(init_loopback, net);
