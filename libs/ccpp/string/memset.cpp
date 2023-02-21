#include <di/prelude.h>

extern "C" unsigned char* memset(unsigned char* dest, int ch, size_t count) {
    auto fill = (unsigned char) ch;
    di::fill(dest, dest + count, fill);
    return dest;
}
