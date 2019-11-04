#ifndef _KERNEL_NET_IP_ADDRESS_H
#define _KERNEL_NET_IP_ADDRESS_H 1

struct ip_v4_address {
    uint8_t addr[4];
} __attribute__((packed));

#endif /* _KERNEL_NET_IP_ADDRESS_H */