#include <string.h>

#include "di/container/algorithm/prelude.h"

extern "C" auto memchr(void const* haystack, int needle, size_t count) -> void* {
    auto const* haystack_unsigned = static_cast<unsigned char const*>(haystack);
    auto const* result = di::find(haystack_unsigned, haystack_unsigned + count, (unsigned char) needle);
    if (result == haystack_unsigned + count) {
        return nullptr;
    }
    return const_cast<unsigned char*>(result);
}
