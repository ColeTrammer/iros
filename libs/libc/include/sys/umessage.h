#ifndef _SYS_UMESSAGE_H
#define _SYS_UMESSAGE_H 1

#include <stdint.h>

struct umessage {
    uint32_t length;
    uint16_t category;
    uint16_t type;
    char data[0];
};

#endif /* _SYS_UMESSAGE_H */
