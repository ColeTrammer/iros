#pragma once

#include <liim/utilities.h>

namespace LIIM::Container::Allocators {
template<typename T>
struct [[nodiscard]] AllocationInfo {
    T* data { nullptr };
    size_t count { 0 };
};

template<typename T>
concept Allocator = requires(T allocator, size_t count) {
    typename T::Value;

    { allocator.allocate(count) } -> SameAs<AllocationInfo<typename T::Value>>;
    { allocator.deallocate(declval<typename T::Value*>(), count) };
};

template<Allocator T>
using AllocatorValue = T::Value;

template<typename Alloc, typename T>
concept AllocatorOf = Allocator<Alloc> && SameAs<T, AllocatorValue<Alloc>>;
}
