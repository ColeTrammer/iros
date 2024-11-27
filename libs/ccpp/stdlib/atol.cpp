#include <ccpp/bits/atoi_implementation.h>
#include <stdlib.h>

extern "C" auto atol(char const* string) -> long {
    return ccpp::atoi<long>(string);
}
