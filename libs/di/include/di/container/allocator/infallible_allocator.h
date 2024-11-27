#pragma once

#include <di/assert/assert_bool.h>
#include <di/container/algorithm/max.h>
#include <di/container/allocator/allocation_result.h>
#include <di/container/allocator/allocator.h>
#include <di/util/std_new.h>

namespace di::container {
struct InfallibleAllocator {
    static auto allocate(usize size, usize alignment) noexcept -> AllocationResult<> {
        auto* result =
            ::operator new(size, std::align_val_t { container::max(alignment, alignof(void*)) }, std::nothrow);
        DI_ASSERT(result);
        return AllocationResult<> { result, size };
    }

    static void deallocate(void* data, usize size, usize alignment) noexcept {
        ::operator delete(data, size, std::align_val_t { container::max(alignment, alignof(void*)) });
    }
};

static_assert(di::concepts::Allocator<InfallibleAllocator>, "InfallibleAllocator is must model di::Allocator");
}

namespace di {
using container::InfallibleAllocator;
}
