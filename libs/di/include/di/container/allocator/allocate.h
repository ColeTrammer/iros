#pragma once

#include <di/container/allocator/allocation_result.h>
#include <di/function/tag_invoke.h>
#include <di/meta/vocab.h>
#include <di/types/prelude.h>

namespace di::container {
namespace detail {
    struct AllocateFunction {
        template<typename A>
        constexpr concepts::MaybeFallible<AllocationResult<>> auto operator()(A& allocator, usize size,
                                                                              usize alignment) const
        requires(concepts::TagInvocable<AllocateFunction, A&, usize, usize> ||
                 requires { allocator.allocate(size, alignment); })
        {
            if constexpr (concepts::TagInvocable<AllocateFunction, A&, usize, usize>) {
                return function::tag_invoke(*this, allocator, size, alignment);
            } else {
                return allocator.allocate(size, alignment);
            }
        }
    };
}

constexpr inline auto allocate = detail::AllocateFunction {};
}

namespace di {
using container::allocate;
}
