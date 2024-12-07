#pragma once

#include "di/container/allocator/allocate.h"
#include "di/container/allocator/deallocate.h"
#include "di/meta/vocab.h"

namespace di::concepts {
template<typename T>
concept Allocator = requires(T& allocator, void* data, usize size, usize alignment) {
    { di::allocate(allocator, size, alignment) } -> MaybeFallible<AllocationResult<>>;
    di::deallocate(allocator, data, size, alignment);
};

template<typename T>
concept InfallibleAllocator = Allocator<T> && requires(T& allocator, void* data, usize size, usize alignment) {
    { di::allocate(allocator, size, alignment) } -> SameAs<AllocationResult<>>;
};

template<typename T>
concept FallibleAllocator = Allocator<T> && !InfallibleAllocator<T>;
}

namespace di::meta {
template<concepts::Allocator Alloc, typename T = void>
using AllocatorResult = meta::LikeExpected<decltype(di::allocate(util::declval<Alloc&>(), 0, 0)), T>;
}

namespace di {
using concepts::Allocator;

using meta::AllocatorResult;
}
