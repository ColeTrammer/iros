#pragma once

#include <di/assert/assert_bool.h>
#include <di/container/allocator/allocate.h>
#include <di/container/allocator/allocation_result.h>
#include <di/container/allocator/allocator.h>
#include <di/container/allocator/std_allocator.h>
#include <di/function/monad/monad_try.h>
#include <di/math/intcmp/checked.h>
#include <di/meta/vocab.h>
#include <di/platform/prelude.h>
#include <di/vocab/expected/as_fallible.h>
#include <di/vocab/expected/try_infallible.h>
#include <di/vocab/expected/unexpected.h>

namespace di::container {
namespace detail {
    template<typename T>
    struct AllocateManyFunction {
        template<concepts::Allocator Alloc>
        constexpr meta::AllocatorResult<Alloc, AllocationResult<T>> operator()(Alloc& allocator, usize count) const {
            if consteval {
                auto* result = std::allocator<T>().allocate(count);
                return AllocationResult<T> { result, count };
            }

            if constexpr (concepts::FallibleAllocator<Alloc>) {
                auto byte_size = math::Checked(count) * sizeof(T);
                if (byte_size.invalid()) {
                    return vocab::Unexpected(BasicError::ValueTooLarge);
                }

                auto result = DI_TRY(di::allocate(allocator, *byte_size.value(), alignof(T)));
                return AllocationResult<T> { static_cast<T*>(result.data), result.count / sizeof(T) };
            } else {
                auto byte_size = math::Checked(count) * sizeof(T);
                DI_ASSERT(!byte_size.invalid());

                auto result = di::allocate(allocator, *byte_size.value(), alignof(T));
                return AllocationResult<T> { static_cast<T*>(result.data), result.count / sizeof(T) };
            }
        }
    };
}

template<typename T>
constexpr inline auto allocate_many = detail::AllocateManyFunction<T> {};
}

namespace di {
using container::allocate_many;
}
