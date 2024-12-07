#pragma once

#include "di/any/container/any.h"
#include "di/container/allocator/allocator.h"
#include "di/platform/prelude.h"

namespace di::any {
template<concepts::Interface Interface, concepts::Allocator Alloc = platform::DefaultAllocator>
using AnyUnique = Any<Interface, UniqueStorage<Alloc>>;
}

namespace di {
using any::AnyUnique;
}
