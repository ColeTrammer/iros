#include <di/math/prelude.h>
#include <di/util/prelude.h>
#include <stdlib.h>

#include "malloc_block.h"

extern "C" void free(void* pointer) {
    if (!pointer) {
        return;
    }

    auto* block = static_cast<ccpp::MallocBlock*>(pointer) - 1;
    auto true_align = block->block_alignment;
    auto true_block_size = di::align_up(sizeof(ccpp::MallocBlock), true_align);
    auto true_size = block->block_size;

    ::operator delete(reinterpret_cast<byte*>(pointer) - true_block_size, true_size, std::align_val_t { true_align });
}
