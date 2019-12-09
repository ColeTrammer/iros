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
#include <kernel/sched/task_sched.h>
#include <kernel/util/spinlock.h>

static struct network_data *head = NULL;
static struct network_data *tail = NULL;
static spinlock_t lock = SPINLOCK_INITIALIZER;

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

void net_on_incoming_packet(const void *buf, size_t len) {
    struct network_data *new_data = calloc(1, sizeof(struct network_data));
    assert(new_data);
    new_data->buf = buf;
    new_data->len = len;

    spin_lock(&lock);

    insque(new_data, tail);
    if (head == NULL) {
        head = tail = new_data;
    } else {
        tail = new_data;
    }

    spin_unlock(&lock);
}

void net_on_incoming_packet_sync(const void *buf, size_t len) {
    const struct ethernet_packet *packet = buf;
    switch (ntohs(packet->ether_type)) {
        case ETHERNET_TYPE_ARP:
            net_arp_recieve((const struct arp_packet *) packet->payload, len - sizeof(struct ethernet_packet));
            break;
        case ETHERNET_TYPE_IPV4:
            net_ip_v4_recieve((const struct ip_v4_packet *) packet->payload, len - sizeof(struct ethernet_packet));
            break;
        default:
            debug_log("Recived unknown packet: [ %#4X ]\n", ntohs(packet->ether_type));
    }
}

void net_network_task_start() {
    for (;;) {
        struct network_data *data = consume();
        if (data == NULL) {
            kernel_yield();
            barrier();
            continue;
        }

        if (data->len <= sizeof(struct ethernet_packet)) {
            debug_log("Packet was to small\n");
            continue;
        }

        net_on_incoming_packet_sync(data->buf, data->len);
        free(data);
    }
}