#pragma once

#include <di/assert/assert_bool.h>
#include <di/container/allocator/allocation_result.h>
#include <di/platform/prelude.h>
#include <di/vocab/error/status_code_forward_declaration.h>
#include <di/vocab/expected/expected_forward_declaration.h>
#include <di/vocab/expected/unexpected.h>

namespace di::container {
struct FailAllocator {
    constexpr static vocab::Expected<AllocationResult<>, vocab::GenericCode> allocate(usize, usize) {
        return vocab::Unexpected(platform::BasicError::NotEnoughMemory);
    }

    static void deallocate(void*, usize, usize) { DI_ASSERT(false); }
};

constexpr inline auto fail_allocator = FailAllocator {};
}

namespace di {
using container::fail_allocator;
using container::FailAllocator;
}
