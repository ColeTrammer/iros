#ifndef _KERNEL_NET_MAC_H
#define _KERNEL_NET_MAC_H 1

#include <stdint.h>

struct mac_address {
    uint8_t addr[6];
} __attribute__((packed));

#define MAC_BROADCAST ((struct mac_address) { { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } })

#endif /* _KERNEL_NET_MAC_H */