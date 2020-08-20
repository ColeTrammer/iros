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
#include <kernel/net/network_task.h>

static struct list_node interface_list = INIT_LIST(interface_list);

static void add_interface(struct network_interface *interface) {
    list_append(&interface_list, &interface->interface_list);
}

static void generic_recieve(struct network_interface *interface, const void *data, size_t len) {
    (void) interface;

    net_on_incoming_packet(data, len);
}

static void generic_recieve_sync(struct network_interface *interface, const void *data, size_t len) {
    (void) interface;

    net_on_incoming_packet_sync(data, len);
}

struct list_node *net_get_interface_list(void) {
    return &interface_list;
}

struct network_interface *net_get_interface_for_ip(struct ip_v4_address address) {
    struct ip_v4_address loopback = IP_V4_LOOPBACK;
    bool use_loopback = memcmp(&address, &loopback, sizeof(struct ip_v4_address)) == 0;

    struct network_interface *interface = NULL;
    net_for_each_interface(iter) {
        if (iter->type == NETWORK_INTERFACE_LOOPBACK && use_loopback) {
            interface = iter;
            break;
        } else if (iter->type == NETWORK_INTERFACE_ETHERNET && !use_loopback) {
            interface = iter;
            break;
        }
    }

    assert(interface);

    debug_log("Got interface: [ %s, %u.%u.%u.%u ]\n", interface->name, address.addr[0], address.addr[1], address.addr[2], address.addr[3]);
    return interface;
}

struct network_interface *net_create_network_interface(const char *name, int type, struct network_interface_ops *ops, void *data) {
    struct network_interface *interface = calloc(1, sizeof(struct network_interface));
    assert(interface);

    assert(name);
    strcpy(interface->name, name);

    assert(type == NETWORK_INTERFACE_ETHERNET || type == NETWORK_INTERFACE_LOOPBACK);
    interface->type = type;

    if (type == NETWORK_INTERFACE_LOOPBACK) {
        interface->address = IP_V4_LOOPBACK;
        interface->mask = IP_V4_BROADCAST;
        interface->default_gateway = IP_V4_LOOPBACK;
        interface->config_context.state = INITIALIZED;
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
