#pragma once

#include <di/container/allocator/allocate.h>
#include <di/container/allocator/allocation_result.h>
#include <di/container/allocator/allocator.h>
#include <di/container/allocator/std_allocator.h>
#include <di/meta/vocab.h>
#include <di/util/voidify.h>
#include <di/vocab/expected/as_fallible.h>
#include <di/vocab/expected/try_infallible.h>

namespace di::container {
namespace detail {
    template<typename T>
    struct DeallocateOneFunction {
        constexpr void operator()(concepts::Allocator auto& allocator, T* pointer) const {
            if consteval {
                return std::allocator<T>().deallocate(pointer, 1);
            }

            return di::deallocate(allocator, di::voidify(pointer), sizeof(T), alignof(T));
        }
    };
}

template<typename T>
constexpr inline auto deallocate_one = detail::DeallocateOneFunction<T> {};
}

namespace di {
using container::deallocate_one;
}
