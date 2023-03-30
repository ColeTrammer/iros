#include <di/prelude.h>
#include <string.h>

extern "C" char* strcat(char* __restrict dest, char const* __restrict src) {
    auto dest_zstring = di::ZString(dest);
    auto* output = di::next(dest_zstring.begin(), dest_zstring.end()).base();
    strcpy(output, src);
    return dest;
}
