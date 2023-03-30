#include <di/prelude.h>

extern "C" char* strncat(char* __restrict dest, char const* __restrict src, size_t count) {
    auto dest_zstring = di::ZString(dest) | di::take(count);
    auto* output = di::next(dest_zstring.begin(), dest_zstring.end()).base().base();

    count -= (output - dest);
    output = di::copy(di::ZCString(src) | di::take(count), output).out;
    *output = '\0';
    return dest;
}
