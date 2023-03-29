#include <ccpp/bits/config.h>
#include <di/prelude.h>

extern "C" unsigned char* memcpy(unsigned char* __restrict dest, unsigned char const* __restrict src, size_t count) {
    di::copy(src, src + count, dest);
    return dest;
}
