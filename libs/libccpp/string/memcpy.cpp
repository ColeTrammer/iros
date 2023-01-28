#include <ccpp/bits/config.h>
#include <di/prelude.h>

extern "C" unsigned char* memcpy(unsigned char* __CCPP_RESTRICT dest, unsigned char const* __CCPP_RESTRICT src,
                                 size_t count) {
    di::copy(src, src + count, dest);
    return dest;
}