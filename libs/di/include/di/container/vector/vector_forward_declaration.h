#pragma once

#include <di/container/allocator/allocator_of.h>
#include <di/platform/prelude.h>

namespace di::container {
template<typename T, concepts::AllocatorOf<T> Alloc = DefaultAllocator<T>>
class Vector;
}
