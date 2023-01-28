#pragma once

#include <liim/container/allocator/allocator.h>
#include <liim/container/allocator/std_allocator.h>

namespace LIIM::Container::Allocators {
template<typename T>
struct StandardAllocator {
    using Value = T;

    constexpr AllocationInfo<T> allocate(size_t count) {
        auto data = std::allocator<T>().allocate(count);
        return { data, count };
    }

    constexpr void deallocate(T* pointer, size_t count) { std::allocator<T>().deallocate(pointer, count); }
};
}
