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

static inline struct ip_v4_address net_ip_v4_mask(struct ip_v4_address address, struct ip_v4_address mask) {
    address.addr[0] &= mask.addr[0];
    address.addr[1] &= mask.addr[1];
    address.addr[2] &= mask.addr[2];
    address.addr[3] &= mask.addr[3];
    return address;
}

static inline int net_ip_v4_equals(struct ip_v4_address a, struct ip_v4_address b) {
    return a.addr[0] == b.addr[0] && a.addr[0] == b.addr[0] && a.addr[1] == b.addr[1] && a.addr[2] == b.addr[2] && a.addr[3] == b.addr[3];
}

#endif /* _KERNEL_NET_IP_ADDRESS_H */
