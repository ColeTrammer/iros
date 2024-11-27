#include <ccpp/bits/strtol_implementation.h>
#include <stdlib.h>

extern "C" auto strtoull(char const* __restrict string, char** __restrict end, int radix) -> unsigned long long {
    return ccpp::strtol<unsigned long long>(string, end, radix);
}
