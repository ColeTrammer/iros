#ifndef _KERNEL_NET_NETWORK_TASK_H
#define _KERNEL_NET_NETWORK_TASK_H 1

#include <stdint.h>

#include <kernel/util/list.h>

struct ethernet_frame;
struct network_interface;
struct packet;

void net_on_incoming_ethernet_frame(const struct ethernet_frame *frame, struct network_interface *interface, size_t len);
void net_on_incoming_packet(struct packet *packet);

void net_network_task_start();

#endif /* _KERNEL_NET_NETWORK_TASK_H */
