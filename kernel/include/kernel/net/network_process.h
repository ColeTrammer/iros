#ifndef _KERNEL_NET_NETWORK_PROCESS_H
#define _KERNEL_NET_NETWORK_PROCESS_H 1

#include <stdint.h>

struct network_data {
    struct network_data *next;
    struct network_data *prev;
    size_t len;
    const void *buf;
};

void net_on_incoming_packet(const void *buf, size_t len);
void net_on_incoming_packet_sync(const void *buf, size_t len);

void net_network_process_start();

#endif /* _KERNEL_NET_NETWORK_PROCESS_H */