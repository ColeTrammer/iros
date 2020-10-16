#pragma once

#include <stdint.h>

namespace IPC {

struct Message {
    uint32_t size;
    uint32_t type;
    uint8_t data[];
};

}
