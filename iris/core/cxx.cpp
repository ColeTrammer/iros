#include <di/container/algorithm/prelude.h>
#include <di/container/string/prelude.h>
#include <di/math/prelude.h>
#include <di/platform/compiler.h>

#if DI_GCC
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
#endif

extern "C" {
int __dso_handle;
}

extern "C" auto strlen(char const* string) -> size_t {
    return di::to_unsigned(di::distance(di::ZCString(string)));
}

extern "C" auto memcpy(unsigned char* __restrict dest, unsigned char const* __restrict src, size_t count)
    -> unsigned char* {
    di::copy(src, src + count, dest);
    return dest;
}

extern "C" auto memset(unsigned char* dest, int ch, size_t count) -> unsigned char* {
    auto fill = (unsigned char) ch;
    di::fill(dest, dest + count, fill);
    return dest;
}
