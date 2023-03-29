#include <stdlib.h>

#include "malloc_block.h"

extern "C" void* malloc(size_t size) {
    return aligned_alloc(alignof(ccpp::MallocBlock), size);
}
