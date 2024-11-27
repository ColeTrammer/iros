#include <ccpp/bits/strtol_implementation.h>
#include <stdlib.h>

extern "C" auto strtoul(char const* __restrict string, char** __restrict end, int radix) -> unsigned long {
    return ccpp::strtol<unsigned long>(string, end, radix);
}
