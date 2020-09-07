#ifndef _KERNEL_NET_LINK_LAYER_ADDRESS_H
#define _KERNEL_NET_LINK_LAYER_ADDRESS_H 1

#include <stdint.h>
#include <string.h>

enum ll_address_type {
    LL_TYPE_NONE,
    LL_TYPE_ETHERNET,
};

struct link_layer_address {
    enum ll_address_type type;
    uint8_t length;
    uint8_t addr[16];
};

#define LINK_LAYER_ADDRESS_NONE                          \
    (struct link_layer_address) {                        \
        .type = LL_TYPE_NONE, .length = 0, .addr = { 0 } \
    }

static inline int net_link_layer_address_equals(struct link_layer_address a, struct link_layer_address b) {
    return a.type == b.type && a.length == b.length && memcmp(a.addr, b.addr, a.length) == 0;
}

#endif /* _KERNEL_NET_LINK_LAYER_ADDRESS_H */
