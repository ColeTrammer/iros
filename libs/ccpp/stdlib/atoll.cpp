#include <ccpp/bits/atoi_implementation.h>
#include <stdlib.h>

extern "C" long long atoll(char const* string) {
    return ccpp::atoi<long long>(string);
}
