#include <di/assert/assert_bool.h>
#include <di/bit/operation/popcount.h>
#include <di/container/interface/empty.h>
#include <di/function/chain.h>
#include <di/function/monad/monad_try.h>
#include <di/function/not_fn.h>
#include <di/math/align_up.h>
#include <di/util/construct_at.h>
#include <di/util/voidify.h>
#include <di/vocab/tuple/tie.h>
#include <dius/runtime/allocate.h>
#include <dius/system/system_call.h>

namespace dius::runtime {
auto const page_size = 4096;

static auto platform_allocate(usize size) -> di::Result<void*> {
#ifdef DIUS_PLATFORM_IROS
    auto result = dius::system::system_call<uptr>(dius::system::Number::allocate_memory, size);
    if (!result) {
        return nullptr;
    }
    return reinterpret_cast<void*>(*result);
#elifdef DIUS_PLATFORM_LINUX
    return system::system_call<uptr>(system::Number::mmap, nullptr, size, PROT_READ | PROT_WRITE,
                                     MAP_ANONYMOUS | MAP_PRIVATE, -1, 0) %
           [](uptr ptr) {
               return reinterpret_cast<void*>(ptr);
           };
#endif
}

static void platform_deallocate(void* pointer, usize size) {
#ifdef DIUS_PLATFORM_IROS
    (void) pointer;
    (void) size;
#elifdef DIUS_PLATFORM_LINUX
    (void) system::system_call<uptr>(system::Number::munmap, pointer, size);
#endif
}

namespace detail {
    auto FreeListList::allocate(usize size, usize) -> di::Result<di::AllocationResult<>> {
        auto result = di::find_if(m_list, di::chain(&FreeList::list, di::not_fn(di::empty)));

        if (result != m_list.end()) {
            auto& free_list = *result;
            return di::AllocationResult<> { free_list.take_node(), size };
        }

        // Allocate a new block.
        auto const block_size = 2zu * 1024zu * 1024zu;
        auto* pointer = TRY(platform_allocate(block_size));
        auto* block = di::construct_at(static_cast<FreeList*>(pointer));

        for (auto offset = size; offset < block_size; offset += size) {
            block->add_node(di::voidify(static_cast<byte*>(pointer) + offset));
        }

        m_list.push_front(*block);
        return di::AllocationResult<> { block->take_node(), size };
    }

    void FreeListList::deallocate(void* pointer) {
        m_list.front()->add_node(pointer);
    }
}

auto Heap::the() -> Heap& {
    static Heap heap;
    return heap;
}

auto Heap::allocate(usize size, usize align) -> di::Result<di::AllocationResult<>> {
    if (di::popcount(align) != 1 || align > page_size) {
        return di::Unexpected(dius::PosixError::InvalidArgument);
    }

    if (size == 0) {
        return di::AllocationResult<> { nullptr, 0 };
    }

    auto block_index = get_block_index(size, align);
    if (!block_index) {
        size = di::align_up(size, page_size);

        auto* result = TRY(platform_allocate(size));
        return di::AllocationResult<> { result, size };
    }

    di::tie(size, align) = block_sizes[*block_index];

    return m_blocks.with_lock([&](auto& blocks) -> di::Result<di::AllocationResult<>> {
        return blocks[*block_index].allocate(size, align);
    });
}

void Heap::deallocate(void* pointer, usize size, usize align) {
    if (!pointer) {
        return;
    }

    ASSERT(di::popcount(align) == 1 && align <= page_size);

    auto block_index = get_block_index(size, align);
    if (!block_index) {
        size = di::align_up(size, page_size);
        platform_deallocate(pointer, size);
        return;
    }

    m_blocks.with_lock([&](auto& blocks) {
        blocks[*block_index].deallocate(pointer);
    });
}
}
