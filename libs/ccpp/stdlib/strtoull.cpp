#include <ccpp/bits/strtol_implementation.h>
#include <stdlib.h>

extern "C" unsigned long long strtoull(char const* __restrict string, char** __restrict end, int radix) {
    return ccpp::strtol<unsigned long long>(string, end, radix);
}
