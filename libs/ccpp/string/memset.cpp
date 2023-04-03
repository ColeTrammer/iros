#include <di/prelude.h>
#include <string.h>

extern "C" void* memset(void* dest, int ch, size_t count) {
    auto* dest_typed = (unsigned char*) dest;
    auto fill = (unsigned char) ch;
    di::fill(dest_typed, dest_typed + count, fill);
    return dest;
}
