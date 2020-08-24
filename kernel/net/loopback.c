#include <stddef.h>

#include <kernel/hal/output.h>
#include <kernel/net/interface.h>
#include <kernel/net/loopback.h>
#include <kernel/net/mac.h>

static int loop_send_ip_v4(struct network_interface *interface, struct route_cache_entry *route, const struct ip_v4_packet *packet,
                           size_t len) {
    (void) route;

    interface->ops->recieve_ip_v4(interface, packet, len);
    return 0;
}

static struct network_interface_ops ops = { .send_ip_v4 = loop_send_ip_v4 };

void init_loopback() {
    net_create_network_interface("lo", NETWORK_INTERFACE_LOOPBACK, &ops, NULL);
}
