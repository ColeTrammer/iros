#include <stdlib.h>
#include <string.h>

#include "malloc_block.h"

extern "C" void* realloc(void* pointer, size_t new_size) {
    // If pointer is NULL, this is simply a call to malloc().
    if (!pointer) {
        return malloc(new_size);
    }

    // See if the current block has room to expand.
    auto* block = static_cast<ccpp::MallocBlock*>(pointer) - 1;
    auto old_size = block->block_size;
    if (old_size >= new_size) {
        return pointer;
    }

    // Now, allocate a new block and copy the contents.
    auto* result = malloc(new_size);
    if (!result) {
        return nullptr;
    }

    // NOTE: old_size is guaranteed to be smaller than new_size, so this does not cause a buffer overrun.
    memcpy(result, pointer, old_size);
    free(pointer);
    return result;
}
