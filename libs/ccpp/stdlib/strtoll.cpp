#include <ccpp/bits/strtol_implementation.h>
#include <stdlib.h>

extern "C" long long strtoll(char const* __restrict string, char** __restrict end, int radix) {
    return ccpp::strtol<long long>(string, end, radix);
}
