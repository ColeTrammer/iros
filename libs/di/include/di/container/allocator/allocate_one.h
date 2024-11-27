#pragma once

#include <di/container/allocator/allocate.h>
#include <di/container/allocator/allocation_result.h>
#include <di/container/allocator/allocator.h>
#include <di/container/allocator/std_allocator.h>
#include <di/meta/vocab.h>
#include <di/vocab/expected/as_fallible.h>
#include <di/vocab/expected/try_infallible.h>

namespace di::container {
namespace detail {
    template<typename T>
    struct AllocateOneFunction {
        template<concepts::Allocator Alloc>
        constexpr auto operator()(Alloc& allocator) const -> meta::AllocatorResult<Alloc, T*> {
            if consteval {
                return std::allocator<T>().allocate(1);
            }

            return vocab::as_fallible(di::allocate(allocator, sizeof(T), alignof(T))) % [](AllocationResult<> result) {
                return static_cast<T*>(result.data);
            } | vocab::try_infallible;
        }
    };
}

template<typename T>
constexpr inline auto allocate_one = detail::AllocateOneFunction<T> {};
}

namespace di {
using container::allocate_one;
}
