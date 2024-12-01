#pragma once

#include <di/assert/assert_bool.h>
#include <di/container/algorithm/max.h>
#include <di/container/allocator/allocation_result.h>
#include <di/platform/custom.h>
#include <di/util/std_new.h>
#include <di/vocab/error/status_code_forward_declaration.h>
#include <di/vocab/expected/expected_forward_declaration.h>
#include <di/vocab/expected/unexpected.h>

namespace di::container {
struct FallibleAllocator {
    static auto allocate(usize size, usize alignment) noexcept
        -> vocab::Expected<AllocationResult<>, vocab::GenericCode> {
        auto* result =
            ::operator new(size, std::align_val_t { container::max(alignment, alignof(void*)) }, std::nothrow);
        if (!result) {
            return vocab::Unexpected(platform::BasicError::NotEnoughMemory);
        }
        return AllocationResult<> { result, size };
    }

    static void deallocate(void* data, usize size, usize alignment) noexcept {
        ::operator delete(data, size, std::align_val_t { container::max(alignment, alignof(void*)) });
    }
};
}

namespace di {
using container::FallibleAllocator;
}
