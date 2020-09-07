#ifndef _KERNEL_NET_NETWORK_TASK_H
#define _KERNEL_NET_NETWORK_TASK_H 1

#include <stdint.h>

#include <kernel/util/list.h>

struct arp_packet;
struct ethernet_frame;
struct ip_v4_packet;

enum network_data_type {
    NETWORK_DATA_ETHERNET,
    NETWORK_DATA_ARP,
    NETWORK_DATA_IP_V4,
};

struct network_data {
    struct list_node list;
    size_t len;
    enum network_data_type type;
    union {
        struct arp_packet *arp_packet;
        struct ethernet_frame *ethernet_frame;
        struct ip_v4_packet *ip_v4_packet;
    };
};

void net_on_incoming_ethernet_frame(const struct ethernet_frame *frame, size_t len);
void net_on_incoming_network_data(struct network_data *data);

void net_network_task_start();

#endif /* _KERNEL_NET_NETWORK_TASK_H */
