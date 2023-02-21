#include <di/prelude.h>

extern "C" unsigned char* memmove(unsigned char* dest, unsigned char const* src, size_t count) {
    if (dest < src) {
        di::copy(src, src + count, dest);
    } else {
        di::copy_backward(src, src + count, dest + count);
    }
    return dest;
}
