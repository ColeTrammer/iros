#include <arpa/inet.h>
#include <assert.h>
#include <search.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/arp.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/interface.h>
#include <kernel/net/ip.h>

static struct network_interface *interfaces = NULL;

static void add_interface(struct network_interface *interface) {
    insque(interface, interfaces);

    if (interfaces == NULL) {
        interfaces = interface;
    }
}

static void generic_recieve(struct network_interface *interface, void *data, size_t len) {
    struct ethernet_packet *packet = data;
    switch (ntohs(packet->ether_type)) {
        case ETHERNET_TYPE_ARP:
            assert(len >= sizeof(struct ethernet_packet) + sizeof(struct arp_packet));
            net_arp_recieve((struct arp_packet*) packet->payload);
            break;
        case ETHERNET_TYPE_IPV4:
            assert(len >= sizeof(struct ethernet_packet) + sizeof(struct ip_v4_packet));
            net_ip_v4_recieve((struct ip_v4_packet*) packet->payload);
            break;
        default:
            debug_log("Recived unknown packet: [ %s, %#4X ]\n", interface->name, ntohs(packet->ether_type));
    }
}

struct network_interface *net_get_interface_for_ip(struct ip_v4_address address) {
    (void) address;
    return interfaces;
}

void net_for_each_interface(void (*func)(struct network_interface *interface)) {
    struct network_interface *interface = interfaces;
    while (interface) {
        func(interface);
        interface = interface->next;
    }
}

struct network_interface *net_create_network_interface(const char *name, int type, struct network_interface_ops *ops, void *data) {
    struct network_interface *interface = malloc(sizeof(struct network_interface));
    assert(interface);

    assert(name);
    strcpy(interface->name, name);

    assert(type == NETWORK_INTERFACE_ETHERNET || type == NETWORK_INTERFACE_LOOPBACK);
    interface->type = type;

    interface->address = (struct ip_v4_address) { { 10, 0, 2, 15 } };
    interface->mask = (struct ip_v4_address) { { 255, 255, 255, 0 } };
    interface->broadcast = (struct ip_v4_address) { { 10, 0, 2, 2 } };

    assert(ops);
    assert(!ops->recieve);
    ops->recieve = generic_recieve;

    interface->ops = ops;
    interface->private_data = data;

    add_interface(interface);

    return interface;
}