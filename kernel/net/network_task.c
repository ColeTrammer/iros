#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/net/arp.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/icmp.h>
#include <kernel/net/ip.h>
#include <kernel/net/network_task.h>
#include <kernel/net/packet.h>
#include <kernel/net/socket.h>
#include <kernel/net/tcp.h>
#include <kernel/net/udp.h>
#include <kernel/proc/task.h>
#include <kernel/proc/wait_queue.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/init.h>
#include <kernel/util/spinlock.h>

static struct task *network_task;

static struct list_node recv_list = INIT_LIST(recv_list);
static spinlock_t lock = SPINLOCK_INITIALIZER;

static struct wait_queue net_wait_queue = WAIT_QUEUE_INITIALIZER;

static struct packet *consume() {
    spin_lock(&lock);

    wait_for_with_spinlock(get_current_task(), !list_is_empty(&recv_list), &net_wait_queue, &lock);

    struct packet *packet = list_first_entry(&recv_list, struct packet, queue);
    list_remove(&packet->queue);
    spin_unlock(&lock);
    return packet;
}

static void enqueue_packet(struct packet *packet) {
    spin_lock(&lock);

    list_append(&recv_list, &packet->queue);

    // Unblock ourselves once we have data
    wake_up_all(&net_wait_queue);
    spin_unlock(&lock);
}

void net_on_incoming_ethernet_frame(const struct ethernet_frame *frame, struct network_interface *interface, size_t len) {
    struct packet *packet = net_create_packet(interface, NULL, NULL, 0);
    assert(packet);

    packet->header_count = 1;
    packet->total_length = len;

    struct packet_header *ethernet_header = &packet->headers[0];
    ethernet_header->flags |= PHF_INITIALIZED;
    ethernet_header->length = len;
    ethernet_header->type = PH_ETHERNET;

    // For now just store a dangling pointer to the ethernet frame. Ethernet drivers
    // have their own persistent buffers for incoming packets. This is far from ideal
    // since the frame's lifetime is not communicated in any way (the space should be
    // marked unusable to prevent the device from overwriting its data).
    ethernet_header->raw_header = (void *) frame;

    enqueue_packet(packet);
}

void net_on_incoming_packet(struct packet *packet) {
    // In this case, the incoming packet was sent via the loopback interface, and
    // can be put into the recieve queue without any modification (except to remove
    // its reference to its sending socket).
    if (packet->socket) {
        net_drop_socket(packet->socket);
        packet->socket = NULL;
    }
    enqueue_packet(packet);
}

void net_network_task_start() {
    for (;;) {
        struct packet *packet = consume();
        assert(packet);

        struct packet_header *header = net_packet_inner_header(packet);
        switch (header->type) {
            case PH_ETHERNET:
                net_ethernet_recieve(packet);
                break;
            case PH_IP_V4:
                net_ip_v4_recieve(packet);
                break;
            case PH_ARP:
                net_arp_recieve(packet);
                break;
            case PH_ICMP:
                net_icmp_recieve(packet);
                break;
            case PH_UDP:
                net_udp_recieve(packet);
                break;
            case PH_TCP:
                net_tcp_recieve(packet);
                break;
            default:
                debug_log("Bad packet header type: [ %s ]\n", net_packet_header_type_to_string(header->type));
                break;
        }
        net_free_packet(packet);
    }
}

static void init_network_task(void) {
    network_task = load_kernel_task((uintptr_t) net_network_task_start, "net");
    assert(network_task);

    sched_add_task(network_task);
}
INIT_FUNCTION(init_network_task, net);
