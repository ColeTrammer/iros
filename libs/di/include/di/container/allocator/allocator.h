#pragma once

#include <di/container/allocator/allocation.h>
#include <di/container/allocator/std_allocator.h>
#include <di/types/prelude.h>

namespace di::container {
template<typename T>
class Allocator {
public:
    using Value = T;

    constexpr Allocation<T> allocate(size_t count) const {
        auto* data = std::allocator<T>().allocate(count);
        DI_ASSERT(data);
        return { data, count };
    }

    constexpr void deallocate(T* data, size_t count) const { std::allocator<T>().deallocate(data, count); }
};
}
