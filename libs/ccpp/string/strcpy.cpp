#include <string.h>

#include "di/container/algorithm/prelude.h"
#include "di/container/string/prelude.h"

extern "C" auto strcpy(char* __restrict dest, char const* __restrict src) -> char* {
    auto* end = di::copy(di::ZCString(src), dest).out;
    *end = '\0';
    return dest;
}
