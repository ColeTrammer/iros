#include <di/container/algorithm/prelude.h>
#include <string.h>

extern "C" void* memcpy(void* __restrict dest, void const* __restrict src, size_t count) {
    auto* dest_typed = (unsigned char*) dest;
    auto* src_typed = (unsigned char const*) src;
    di::copy(src_typed, src_typed + count, dest_typed);
    return dest;
}
