#include <di/assert/prelude.h>
#include <di/util/prelude.h>
#include <stdlib.h>

extern "C" void abort() {
    DI_ASSERT(false);
    di::unreachable();
}
