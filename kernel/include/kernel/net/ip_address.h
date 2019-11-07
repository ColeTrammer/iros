#ifndef _KERNEL_NET_IP_ADDRESS_H
#define _KERNEL_NET_IP_ADDRESS_H 1

#include <stdint.h>

struct ip_v4_address {
    uint8_t addr[4];
} __attribute__((packed));

static inline struct ip_v4_address ip_v4_from_uint(uint32_t uint) {
    return (struct ip_v4_address) { { (uint & 0x000000FF) >> 0,
                                      (uint & 0x0000FF00) >> 8,
                                      (uint & 0x00FF0000) >> 16,
                                      (uint & 0xFF000000) >> 24 
    } };
}

#endif /* _KERNEL_NET_IP_ADDRESS_H */