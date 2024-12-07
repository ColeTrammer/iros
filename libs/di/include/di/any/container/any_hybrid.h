#pragma once

#include "di/any/container/any.h"
#include "di/any/storage/storage_category.h"
#include "di/container/allocator/allocator.h"
#include "di/platform/prelude.h"

namespace di::any {
template<concepts::Interface Interface, StorageCategory storage_category = StorageCategory::MoveOnly,
         size_t inline_size = 2 * sizeof(void*), size_t inline_align = alignof(void*),
         concepts::Allocator Alloc = platform::DefaultAllocator>
using AnyHybrid = Any<Interface, HybridStorage<storage_category, inline_size, inline_align, Alloc>>;
}

namespace di {
using any::AnyHybrid;
}
