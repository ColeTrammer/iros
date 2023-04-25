#include <ccpp/bits/atoi_implementation.h>
#include <stdlib.h>

extern "C" long atol(char const* string) {
    return ccpp::atoi<long>(string);
}
