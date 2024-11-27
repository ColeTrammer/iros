#include <di/assert/prelude.h>
#include <di/util/prelude.h>
#include <time.h>

extern "C" auto strftime(char* __restrict, size_t, char const* __restrict, const struct tm* __restrict) -> size_t {
    ASSERT(false);
    di::unreachable();
}
