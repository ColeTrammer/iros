#include <ccpp/bits/atoi_implementation.h>
#include <stdlib.h>

extern "C" auto atoi(char const* string) -> int {
    return ccpp::atoi<int>(string);
}
