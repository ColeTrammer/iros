#pragma once

#include "di/container/allocator/allocator.h"
#include "di/platform/prelude.h"

namespace di::container {
template<typename T, concepts::Allocator Alloc = DefaultAllocator>
class Vector;
}
