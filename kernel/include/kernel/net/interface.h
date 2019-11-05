#ifndef _KERNEL_NET_INTERFACE_H
#define _KERNEL_NET_INTERFACE_H 1

#include <sys/types.h>

#include <kernel/net/ip_address.h>
#include <kernel/net/mac.h>

struct network_interface;

struct network_interface_ops {
    ssize_t (*send)(struct network_interface *interface, const void *data, size_t len);
    void (*recieve)(struct network_interface *interface, void *data, size_t len);
    struct mac_address (*get_mac_address)(struct network_interface *interface);
};

#define NETWORK_INTERFACE_ETHERNET 1
#define NETWORK_INTERFACE_LOOPBACK 2

struct network_interface {
    struct network_interface *next;
    struct network_interface *prev;

    char name[8];
    int type;

    struct ip_v4_address address;
    struct ip_v4_address mask;
    struct ip_v4_address broadcast;

    struct network_interface_ops *ops;

    void *private_data;
};

void net_for_each_interface(void (*func)(struct network_interface *interface));
struct network_interface *net_create_network_interface(const char *name, int type, struct network_interface_ops *ops, void *data);

#endif /* _KERNEL_NET_INTERFACE_H */