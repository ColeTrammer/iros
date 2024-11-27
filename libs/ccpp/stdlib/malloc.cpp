#include <stdlib.h>

#include "malloc_block.h"

extern "C" auto malloc(size_t size) -> void* {
    return aligned_alloc(alignof(ccpp::MallocBlock), size);
}
