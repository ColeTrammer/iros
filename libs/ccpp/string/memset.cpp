#include <string.h>

#include "di/container/algorithm/prelude.h"

extern "C" auto memset(void* dest, int ch, size_t count) -> void* {
    auto* dest_typed = (unsigned char*) dest;
    auto fill = (unsigned char) ch;
    di::fill(dest_typed, dest_typed + count, fill);
    return dest;
}
