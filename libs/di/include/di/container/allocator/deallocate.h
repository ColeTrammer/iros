#pragma once

#include "di/function/tag_invoke.h"
#include "di/types/prelude.h"

namespace di::container {
namespace detail {
    struct DeallocateFunction {
        template<typename A>
        constexpr void operator()(A& allocator, void* data, usize size, usize alignment) const
        requires(concepts::TagInvocable<DeallocateFunction, A&, void*, usize, usize> ||
                 requires { allocator.deallocate(data, size, alignment); })
        {
            if constexpr (concepts::TagInvocable<DeallocateFunction, A&, void*, usize, usize>) {
                (void) function::tag_invoke(*this, allocator, data, size, alignment);
            } else {
                (void) allocator.deallocate(data, size, alignment);
            }
        }
    };
}

constexpr inline auto deallocate = detail::DeallocateFunction {};
}

namespace di {
using container::deallocate;
}
