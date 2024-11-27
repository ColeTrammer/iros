#include <di/container/algorithm/prelude.h>
#include <di/container/string/prelude.h>
#include <di/math/prelude.h>
#include <di/platform/compiler.h>
#include <di/util/prelude.h>

#if DI_GCC
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
#endif

extern "C++" {
[[gnu::weak]] void* __dso_handle;
}

extern "C" [[gnu::weak]] auto strlen(char const* string) -> size_t {
    return di::to_unsigned(di::distance(di::ZCString(string)));
}

extern "C" [[gnu::weak]] auto memcpy(unsigned char* __restrict dest, unsigned char const* __restrict src, size_t count)
    -> unsigned char* {
    di::copy(src, src + count, dest);
    return dest;
}

extern "C" [[gnu::weak]] auto memset(unsigned char* dest, int ch, size_t count) -> unsigned char* {
    auto fill = (unsigned char) ch;
    di::fill(dest, dest + count, fill);
    return dest;
}

extern "C" [[gnu::weak]] auto memmove(void* dest, void const* src, size_t count) -> void* {
    auto* dest_typed = (unsigned char*) dest;
    auto const* src_typed = (unsigned char const*) src;
    if (di::to_uintptr(dest_typed) < di::to_uintptr(src_typed)) {
        di::copy(src_typed, src_typed + count, dest_typed);
    } else {
        di::copy_backward(src_typed, src_typed + count, dest_typed + count);
    }
    return dest;
}
