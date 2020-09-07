#include <stddef.h>

#include <kernel/hal/output.h>
#include <kernel/net/interface.h>
#include <kernel/net/loopback.h>
#include <kernel/net/mac.h>

static int loop_send_ip_v4(struct network_interface *interface, struct destination_cache_entry *route, struct network_data *data) {
    (void) route;

    net_recieve_network_data(interface, data);
    return 0;
}

static struct network_interface_ops ops = { .send_ip_v4 = loop_send_ip_v4 };

void init_loopback() {
    net_create_network_interface("lo", NETWORK_INTERFACE_LOOPBACK, &ops, NULL);
}
