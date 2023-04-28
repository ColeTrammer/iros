#include <di/assert/prelude.h>
#include <di/util/prelude.h>
#include <stdlib.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/strtod.html
extern "C" double strtod(char const* __restrict, char** __restrict) {
    ASSERT(false);
    di::unreachable();
}
