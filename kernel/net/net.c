#include <kernel/hal/output.h>
#include <kernel/net/arp.h>
#include <kernel/net/dhcp.h>
#include <kernel/net/interface.h>
#include <kernel/net/loopback.h>
#include <kernel/net/mac.h>
#include <kernel/net/net.h>
#include <kernel/net/network_task.h>
#include <kernel/net/port.h>
#include <kernel/net/route_cache.h>
#include <kernel/net/socket.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>

struct task *network_task;

void init_net() {
    init_loopback();
    init_net_sockets();
    init_mac();
    init_ports();
    init_route_cache();

    network_task = load_kernel_task((uintptr_t) net_network_task_start, "net");
    assert(network_task);

    sched_add_task(network_task);

    net_for_each_interface(interface) {
        debug_log("Initializing interface: [ %s ]\n", interface->name);

        if (interface->type != NETWORK_INTERFACE_LOOPBACK) {
            net_configure_interface_with_dhcp(interface);
        }
    }
}
