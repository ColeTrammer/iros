#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/arp.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/ip.h>
#include <kernel/net/network_task.h>
#include <kernel/net/socket.h>
#include <kernel/proc/wait_queue.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/spinlock.h>

extern struct task *network_task;

static struct list_node recv_list = INIT_LIST(recv_list);
static spinlock_t lock = SPINLOCK_INITIALIZER;

static struct wait_queue net_wait_queue = WAIT_QUEUE_INITIALIZER;

static struct network_data *consume() {
    spin_lock(&lock);

    if (list_is_empty(&recv_list)) {
        spin_unlock(&lock);
        return NULL;
    }

    struct network_data *data = list_first_entry(&recv_list, struct network_data, list);
    list_remove(&data->list);
    spin_unlock(&lock);
    return data;
}

static void enqueue_packet(struct network_data *data) {
    spin_lock(&lock);

    list_append(&recv_list, &data->list);

    // Unblock ourselves once we have data
    wake_up_all(&net_wait_queue);
    spin_unlock(&lock);
}

void net_free_network_data(struct network_data *data) {
    if (data->socket) {
        net_drop_socket(data->socket);
    }
    free(data);
}

void net_on_incoming_ethernet_frame(const struct ethernet_frame *frame, size_t len) {
    struct network_data *new_data = malloc(sizeof(struct network_data));
    assert(new_data);

    // For now just store a dangling pointer to the ethernet frame. Ethernet drivers
    // have their own persistent buffers for incoming packets. This is far from ideal
    // since the frame's lifetime is not communicated in any way (the space should be
    // marked unusable to prevent the device from overwriting its data).
    new_data->socket = NULL;
    new_data->ethernet_frame = (struct ethernet_frame *) frame;
    new_data->len = len;
    new_data->type = NETWORK_DATA_ETHERNET;

    enqueue_packet(new_data);
}

void net_on_incoming_network_data(struct network_data *data) {
    // In this case, the incoming packet was sent via the loopback interface, and
    // can be put into the recieve queue without any modification (except to remove
    // its reference to its sending socket).
    if (data->socket) {
        net_drop_socket(data->socket);
        data->socket = NULL;
    }
    enqueue_packet(data);
}

void net_network_task_start() {
    for (;;) {
        struct network_data *data = consume();
        if (data == NULL) {
            wait_on(&net_wait_queue);
            continue;
        }

        if (data->len <= sizeof(struct ethernet_frame)) {
            debug_log("Packet was too small\n");
            continue;
        }

        switch (data->type) {
            case NETWORK_DATA_ETHERNET:
                net_ethernet_recieve(data->ethernet_frame, data->len);
                break;
            case NETWORK_DATA_IP_V4:
                net_ip_v4_recieve(data->ip_v4_packet, data->len);
                break;
            default:
                assert(false);
                break;
        }
        net_free_network_data(data);
    }
}
