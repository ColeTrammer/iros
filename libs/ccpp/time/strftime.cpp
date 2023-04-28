#include <di/assert/prelude.h>
#include <di/util/prelude.h>
#include <time.h>

extern "C" size_t strftime(char* __restrict, size_t, char const* __restrict, const struct tm* __restrict) {
    ASSERT(false);
    di::unreachable();
}
