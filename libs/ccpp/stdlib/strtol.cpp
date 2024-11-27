#include <ccpp/bits/strtol_implementation.h>
#include <stdlib.h>

extern "C" auto strtol(char const* __restrict string, char** __restrict end, int radix) -> long {
    return ccpp::strtol<long>(string, end, radix);
}
