#pragma once

#include <di/concepts/language_void.h>
#include <di/concepts/maybe_fallible.h>
#include <di/container/allocator/allocation.h>
#include <di/types/prelude.h>

namespace di::concepts {
template<typename Alloc, typename T>
concept AllocatorOf = requires(Alloc& allocator, T* data, size_t count) {
    typename Alloc::Value;
    { allocator.allocate(count) } -> MaybeFallible<container::Allocation<T>>;
    { allocator.deallocate(data, count) } -> LanguageVoid;
};
}
