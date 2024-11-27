#include <ccpp/bits/strtol_implementation.h>
#include <stdlib.h>

extern "C" auto strtoll(char const* __restrict string, char** __restrict end, int radix) -> long long {
    return ccpp::strtol<long long>(string, end, radix);
}
