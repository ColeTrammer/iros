#include <ccpp/bits/atoi_implementation.h>
#include <stdlib.h>

extern "C" int atoi(char const* string) {
    return ccpp::atoi<int>(string);
}
