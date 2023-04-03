#include <di/prelude.h>
#include <string.h>

extern "C" void* memmove(void* dest, void const* src, size_t count) {
    auto* dest_typed = (unsigned char*) dest;
    auto const* src_typed = (unsigned char const*) src;
    if (di::to_uintptr(dest_typed) < di::to_uintptr(src_typed)) {
        di::copy(src_typed, src_typed + count, dest_typed);
    } else {
        di::copy_backward(src_typed, src_typed + count, dest_typed + count);
    }
    return dest;
}
