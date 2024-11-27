#include <ccpp/bits/atoi_implementation.h>
#include <stdlib.h>

extern "C" auto atoll(char const* string) -> long long {
    return ccpp::atoi<long long>(string);
}
