#include <di/assert/prelude.h>
#include <di/util/prelude.h>
#include <stdio.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/ungetc.html
extern "C" int ungetc(int, FILE*) {
    ASSERT(false);
    di::unreachable();
}
