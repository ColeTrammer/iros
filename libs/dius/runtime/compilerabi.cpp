#include <di/container/algorithm/prelude.h>
#include <di/container/string/prelude.h>
#include <di/math/prelude.h>

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
#endif

extern "C++" {
[[gnu::weak]] void* __dso_handle;
}

extern "C" [[gnu::weak]] size_t strlen(char const* string) {
    return di::to_unsigned(di::distance(di::ZCString(string)));
}

extern "C" [[gnu::weak]] unsigned char* memcpy(unsigned char* __restrict dest, unsigned char const* __restrict src,
                                               size_t count) {
    di::copy(src, src + count, dest);
    return dest;
}

extern "C" [[gnu::weak]] unsigned char* memset(unsigned char* dest, int ch, size_t count) {
    auto fill = (unsigned char) ch;
    di::fill(dest, dest + count, fill);
    return dest;
}
