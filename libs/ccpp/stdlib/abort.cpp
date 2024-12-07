#include <stdlib.h>

#include "di/assert/prelude.h"
#include "di/util/prelude.h"

extern "C" void abort() {
    DI_ASSERT(false);
    di::unreachable();
}
