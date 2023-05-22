#pragma once

#include <di/any/container/any.h>
#include <di/container/allocator/allocator.h>
#include <di/platform/prelude.h>

namespace di::any {
template<concepts::Interface Interface, size_t inline_size = 2 * sizeof(void*), size_t inline_align = alignof(void*),
         concepts::Allocator Alloc = platform::DefaultAllocator>
using AnyHybrid = Any<Interface, HybridStorage<inline_size, inline_align, Alloc>>;
}
