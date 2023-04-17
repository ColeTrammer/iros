#include <di/math/intcmp/prelude.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

extern "C" void* calloc(size_t count, size_t size) {
    auto true_size = di::Checked(count) * size;
    if (!true_size.valid()) {
        errno = EOVERFLOW;
        return nullptr;
    }

    auto* result = malloc(*true_size.value());
    if (!result) {
        return nullptr;
    }
    memset(result, 0, *true_size.value());
    return result;
}
