#include <di/container/algorithm/prelude.h>
#include <string.h>

extern "C" void* memchr(void const* haystack, int needle, size_t count) {
    auto const* haystack_unsigned = static_cast<unsigned char const*>(haystack);
    auto const* result = di::find(haystack_unsigned, haystack_unsigned + count, (unsigned char) needle);
    if (result == haystack_unsigned + count) {
        return nullptr;
    }
    return const_cast<unsigned char*>(result);
}
