#ifndef _KERNEL_NET_MAC_H
#define _KERNEL_NET_MAC_H 1

#include <stdint.h>
#include <string.h>

#include <kernel/net/ip_address.h>
#include <kernel/net/link_layer_address.h>
#include <kernel/util/hash_map.h>

#define MAC_BROADCAST ((struct mac_address) { { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } })

struct hash_map;

struct mac_address {
    uint8_t addr[6];
} __attribute__((packed));

static inline struct mac_address net_link_layer_address_to_mac(struct link_layer_address address) {
    struct mac_address ret;
    memcpy(ret.addr, address.addr, sizeof(struct mac_address));
    return ret;
}

static inline struct link_layer_address net_mac_to_link_layer_address(struct mac_address address) {
    struct link_layer_address ret;
    ret.type = LL_TYPE_ETHERNET;
    ret.length = sizeof(struct mac_address);
    memcpy(ret.addr, address.addr, sizeof(struct mac_address));
    return ret;
}

#endif /* _KERNEL_NET_MAC_H */
