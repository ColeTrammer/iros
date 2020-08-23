#ifndef _KERNEL_NET_NETWORK_TASK_H
#define _KERNEL_NET_NETWORK_TASK_H 1

#include <stdint.h>

struct ethernet_frame;
struct ip_v4_packet;

enum network_data_type {
    NETWORK_DATA_ETHERNET,
    NETWORK_DATA_IP_V4,
};

struct network_data {
    struct network_data *next;
    struct network_data *prev;
    size_t len;
    enum network_data_type type;
    union {
        const struct ethernet_frame *ethernet_frame;
        const struct ip_v4_packet *ip_v4_packet;
    };
};

void net_on_incoming_ethernet_frame(const struct ethernet_frame *frame, size_t len);
void net_on_incoming_ip_v4_packet(const struct ip_v4_packet *packet, size_t len);

void net_network_task_start();

#endif /* _KERNEL_NET_NETWORK_TASK_H */
