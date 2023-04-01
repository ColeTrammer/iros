#include <di/prelude.h>

#ifdef __GCC__
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
#endif

extern "C" {
[[gnu::weak]] int __dso_handle;
}

extern "C" size_t strlen(char const* string) {
    return di::to_unsigned(di::distance(di::ZCString(string)));
}

extern "C" unsigned char* memcpy(unsigned char* __restrict dest, unsigned char const* __restrict src, size_t count) {
    di::copy(src, src + count, dest);
    return dest;
}

extern "C" unsigned char* memset(unsigned char* dest, int ch, size_t count) {
    auto fill = (unsigned char) ch;
    di::fill(dest, dest + count, fill);
    return dest;
}
