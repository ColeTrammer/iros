#include <string.h>

#include "di/container/algorithm/prelude.h"

extern "C" auto memcpy(void* __restrict dest, void const* __restrict src, size_t count) -> void* {
    auto* dest_typed = (unsigned char*) dest;
    auto const* src_typed = (unsigned char const*) src;
    di::copy(src_typed, src_typed + count, dest_typed);
    return dest;
}
