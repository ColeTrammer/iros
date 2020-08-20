#include <arpa/inet.h>
#include <assert.h>
#include <search.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/arp.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/ip.h>
#include <kernel/net/network_task.h>
#include <kernel/proc/wait_queue.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/spinlock.h>

extern struct task *network_task;

static struct network_data *head = NULL;
static struct network_data *tail = NULL;
static spinlock_t lock = SPINLOCK_INITIALIZER;

static struct wait_queue net_wait_queue = WAIT_QUEUE_INITIALIZER;

static struct network_data *consume() {
    spin_lock(&lock);

    if (head == NULL) {
        spin_unlock(&lock);
        return NULL;
    }

    struct network_data *data = head;
    head = head->next;
    remque((void *) data);
    if (head == NULL) {
        tail = NULL;
    }

    spin_unlock(&lock);
    return data;
}

void net_on_incoming_ethernet_frame(const struct ethernet_frame *frame, size_t len) {
    struct network_data *new_data = calloc(1, sizeof(struct network_data));
    assert(new_data);
    new_data->frame = frame;
    new_data->len = len;

    spin_lock(&lock);

    insque(new_data, tail);
    if (head == NULL) {
        head = tail = new_data;
    } else {
        tail = new_data;
    }

    // Unblock ourselves once we have data
    wake_up_all(&net_wait_queue);
    spin_unlock(&lock);
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

        net_ethernet_recieve(data->frame, data->len);
        free(data);
    }
}
