#include <di/container/algorithm/prelude.h>
#include <di/container/string/prelude.h>
#include <di/container/view/prelude.h>
#include <string.h>

extern "C" char* strncat(char* __restrict dest, char const* __restrict src, size_t count) {
    auto dest_zstring = di::ZString(dest);
    auto* output = di::next(dest_zstring.begin(), dest_zstring.end()).base();

    output = di::copy(di::ZCString(src) | di::take(count), output).out;
    *output = '\0';
    return dest;
}
