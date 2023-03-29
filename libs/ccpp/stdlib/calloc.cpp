#include <stdlib.h>
#include <string.h>

extern "C" void* calloc(size_t count, size_t size) {
    // FIXME: check for overflow.
    auto true_size = count * size;

    auto* result = malloc(true_size);
    if (!result) {
        return nullptr;
    }
    memset(result, 0, true_size);
    return result;
}
