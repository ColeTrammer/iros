#include <di/container/algorithm/prelude.h>
#include <di/math/prelude.h>
#include <di/util/prelude.h>
#include <stdlib.h>

#include "malloc_block.h"

extern "C" void* aligned_alloc(size_t alignment, size_t size) {
    auto true_align = di::max(alignment, alignof(ccpp::MallocBlock));
    auto true_block_size = di::align_up(sizeof(ccpp::MallocBlock), true_align);
    auto true_size = size + true_block_size;

    auto* result = ::operator new(true_size, std::align_val_t { true_align }, std::nothrow);
    if (!result) {
        return result;
    }

    auto* block = reinterpret_cast<ccpp::MallocBlock*>(static_cast<byte*>(result) + true_block_size) - 1;
    block->block_size = true_size;
    block->block_alignment = true_align;
    return block + 1;
}
