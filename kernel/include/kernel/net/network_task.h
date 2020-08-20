#ifndef _KERNEL_NET_NETWORK_TASK_H
#define _KERNEL_NET_NETWORK_TASK_H 1

#include <stdint.h>

struct ethernet_frame;

struct network_data {
    struct network_data *next;
    struct network_data *prev;
    size_t len;
    const struct ethernet_frame *frame;
};

void net_on_incoming_ethernet_frame(const struct ethernet_frame *frame, size_t len);

void net_network_task_start();

#endif /* _KERNEL_NET_NETWORK_TASK_H */
