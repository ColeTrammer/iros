#include <di/container/algorithm/prelude.h>
#include <di/container/string/prelude.h>
#include <string.h>

extern "C" auto strcat(char* __restrict dest, char const* __restrict src) -> char* {
    auto dest_zstring = di::ZString(dest);
    auto* output = di::next(dest_zstring.begin(), dest_zstring.end()).base();
    strcpy(output, src);
    return dest;
}
