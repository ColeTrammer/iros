#include <di/prelude.h>
#include <string.h>

extern "C" char* strcpy(char* __restrict dest, char const* __restrict src) {
    auto* end = di::copy(di::ZCString(src), dest).out;
    *end = '\0';
    return dest;
}
