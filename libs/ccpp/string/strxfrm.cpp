#include <di/container/algorithm/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/string/prelude.h>
#include <di/container/view/prelude.h>
#include <string.h>

extern "C" size_t strxfrm(char* __restrict dest, char const* __restrict src, size_t count) {
    // NOTE: This is valid for the "C" locale, but may needed to be changed if other locales are supported.
    if (count == 0) {
        return strlen(src);
    }

    auto [src_end, dest_end] = di::copy(di::ZCString(src) | di::take(count), dest);
    if (dest_end < dest + count) {
        *dest_end = '\0';
    }

    auto end = di::next(src_end.base(), di::default_sentinel);
    return end.base() - src;
}
