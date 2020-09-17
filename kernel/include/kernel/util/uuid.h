#ifndef _KERNEL_UTIL_UUID_H
#define _KERNEL_UTIL_UUID_H 1

#include <stdint.h>
#include <string.h>

#include <kernel/util/macros.h>

struct uuid {
    union {
        uint8_t raw[16];
        struct {
            uint32_t time_low;
            uint16_t time_mid;
            uint16_t time_hi_and_version;
            uint8_t clk_seq_hi_res;
            uint8_t clk_seq_low;
            uint16_t node_low;
            uint32_t node_hi;
        };
    };
};

_Static_assert(sizeof(struct uuid) == 16);

#define UUID_ZEROES \
    (struct uuid) { 0 }

static inline bool uuid_equals(struct uuid a, struct uuid b) {
    return memcmp(&a, &b, sizeof(struct uuid)) == 0;
}

#endif /* _KERNEL_UTIL_UUID_H */
