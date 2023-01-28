#ifndef _KERNEL_UTIL_UUID_H
#define _KERNEL_UTIL_UUID_H 1

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
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

static inline bool uuid_equals(struct uuid a, struct uuid b) {
    return memcmp(&a, &b, sizeof(struct uuid)) == 0;
}

static inline void uuid_to_string(struct uuid uuid, char *buffer, size_t buffer_max) {
    int result =
        snprintf(buffer, buffer_max - 1, "%.8x-%.4x-%.4x-%.2x%.2x-%.4x%.8x", htonl(uuid.time_low), htons(uuid.time_mid),
                 htons(uuid.time_hi_and_version), uuid.clk_seq_hi_res, uuid.clk_seq_low, htons(uuid.node_low), htonl(uuid.node_hi));
    buffer[result] = '\0';
}

#define UUID_ZEROES \
    (struct uuid) { 0 }

#define UUID_STRING_MAX 37

#endif /* _KERNEL_UTIL_UUID_H */
