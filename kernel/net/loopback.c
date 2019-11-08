#include <stddef.h>

#include <kernel/hal/output.h>
#include <kernel/net/interface.h>
#include <kernel/net/loopback.h>
#include <kernel/net/mac.h>

static ssize_t loop_send(struct network_interface *interface, const void *data, size_t len) {
    debug_log("Sending data via loopback\n");

    interface->ops->recieve_sync(interface, data, len);
    return (ssize_t) len;
}

static struct mac_address get_mac(struct network_interface *interface) {
    (void) interface;
    return MAC_ZEROES;
}

static struct network_interface_ops ops = {
    loop_send, NULL, NULL, get_mac
};

void init_loopback() {
    net_create_network_interface("lo", NETWORK_INTERFACE_LOOPBACK, &ops, NULL);
}