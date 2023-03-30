#include <di/prelude.h>

extern "C" char* strncpy(char* __restrict dest, char const* __restrict src, size_t count) {
    auto* end = di::copy(di::ZCString(src) | di::take(count), dest).out;
    di::fill(end, dest + count, '\0');
    return dest;
}
