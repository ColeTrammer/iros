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
#include <kernel/net/network_process.h>

static struct network_interface *interfaces = NULL;

static void add_interface(struct network_interface *interface) {
    insque(interface, interfaces);

    if (interfaces == NULL) {
        interfaces = interface;
    }
}

static void generic_recieve(struct network_interface *interface, const void *data, size_t len) {
    (void) interface;

    net_on_incoming_packet(data, len);
}

static void generic_recieve_sync(struct network_interface *interface, const void *data, size_t len) {
    (void) interface;

    net_on_incoming_packet_sync(data, len);
}

struct network_interface *net_get_interface_for_ip(struct ip_v4_address address) {
    struct ip_v4_address loopback = IP_V4_LOOPBACK;
    struct network_interface *interface = interfaces;
    bool use_loopback = memcmp(&address, &loopback, sizeof(struct ip_v4_address)) == 0;

    while (use_loopback ? interface->type != NETWORK_INTERFACE_LOOPBACK : interface->type != NETWORK_INTERFACE_ETHERNET) {
        interface = interface->next;
    }

    assert(interface);

    debug_log("Got interface: [ %s, %u.%u.%u.%u ]\n", interface->name, address.addr[0], address.addr[1], address.addr[2], address.addr[3]);
    return interface;
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

    if (type == NETWORK_INTERFACE_ETHERNET) {
        interface->address = (struct ip_v4_address) { { 10, 0, 2, 15 } };
        interface->mask = (struct ip_v4_address) { { 255, 255, 255, 0 } };
        interface->broadcast = (struct ip_v4_address) { { 10, 0, 2, 2 } };
    } else {
        interface->address = IP_V4_LOOPBACK;
        interface->mask = (struct ip_v4_address) { { 255, 255, 255, 255 } };
        interface->broadcast = IP_V4_LOOPBACK;
    }

    assert(ops);
    assert(!ops->recieve);
    assert(!ops->recieve_sync);
    ops->recieve = generic_recieve;
    ops->recieve_sync = generic_recieve_sync;

    interface->ops = ops;
    interface->private_data = data;

    add_interface(interface);

    return interface;
}