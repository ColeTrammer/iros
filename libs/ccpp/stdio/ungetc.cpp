#include <stdio.h>

#include "di/assert/prelude.h"
#include "di/util/prelude.h"

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/ungetc.html
extern "C" auto ungetc(int, FILE*) -> int {
    ASSERT(false);
    di::unreachable();
}
