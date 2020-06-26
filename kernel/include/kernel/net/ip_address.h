#ifndef _KERNEL_NET_IP_ADDRESS_H
#define _KERNEL_NET_IP_ADDRESS_H 1

#include <stdint.h>

struct ip_v4_address {
    uint8_t addr[4];
} __attribute__((packed));

#define IP_V4_ZEROES    ((struct ip_v4_address) { { 0, 0, 0, 0 } })
#define IP_V4_LOOPBACK  ((struct ip_v4_address) { { 127, 0, 0, 1 } })
#define IP_V4_BROADCAST ((struct ip_v4_address) { { 255, 255, 255, 255 } })

static inline uint32_t ip_v4_to_uint(struct ip_v4_address addr) {
    return addr.addr[0] | addr.addr[1] << 8 | addr.addr[2] << 16 | addr.addr[3] << 24;
}

static inline struct ip_v4_address ip_v4_from_uint(uint32_t uint) {
    return (struct ip_v4_address) { { (uint & 0x000000FF) >> 0, (uint & 0x0000FF00) >> 8, (uint & 0x00FF0000) >> 16,
                                      (uint & 0xFF000000) >> 24 } };
}

#endif /* _KERNEL_NET_IP_ADDRESS_H */
