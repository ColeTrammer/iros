#include <kernel/hal/output.h>
#include <kernel/net/arp.h>
#include <kernel/net/interface.h>
#include <kernel/net/loopback.h>
#include <kernel/net/mac.h>
#include <kernel/net/net.h>
#include <kernel/net/network_task.h>
#include <kernel/net/port.h>
#include <kernel/net/socket.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>

struct task *network_task;

static void init_ip_v4_mappings(struct network_interface *interface) {
    debug_log("Initializing interface: [ %s ]\n", interface->name);

    net_create_ip_v4_to_mac_mapping(interface->address, interface->ops->get_mac_address(interface));
    if (interface->type != NETWORK_INTERFACE_LOOPBACK) {
        net_create_ip_v4_to_mac_mapping(interface->broadcast, MAC_BROADCAST); // NOTE: This will be updated later by an arp requtest

        net_send_arp_request(interface, interface->broadcast);
    }
}

void init_net() {
    init_loopback();
    init_net_sockets();
    init_mac();
    init_ports();

    network_task = load_kernel_task((uintptr_t) net_network_task_start, "net");
    assert(network_task);

    sched_add_task(network_task);

    net_for_each_interface(init_ip_v4_mappings);
}
