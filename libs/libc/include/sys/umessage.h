#ifndef _SYS_UMESSAGE_H
#define _SYS_UMESSAGE_H 1

#include <stdint.h>

enum umessage_category_number {
    UMESSAGE_INTERFACE,
    UMESSAGE_NUM_CATEGORIES,
};

struct umessage {
    uint32_t length;
    uint16_t category;
    uint16_t type;
    char data[0];
};

#endif /* _SYS_UMESSAGE_H */
