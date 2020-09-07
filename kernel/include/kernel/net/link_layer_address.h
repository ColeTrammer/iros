#ifndef _KERNEL_NET_LINK_LAYER_ADDRESS_H
#define _KERNEL_NET_LINK_LAYER_ADDRESS_H 1

#include <stdint.h>

struct link_layer_address {
    uint8_t length;
    uint8_t addr[16];
};

#endif /* _KERNEL_NET_LINK_LAYER_ADDRESS_H */
