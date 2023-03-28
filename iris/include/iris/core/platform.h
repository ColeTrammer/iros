#pragma once

#include <di/container/algorithm/max.h>
#include <di/container/allocator/allocation.h>
#include <di/container/allocator/forward_declaration.h>
#include <di/types/integers.h>
#include <di/util/std_new.h>
#include <di/vocab/error/prelude.h>
#include <iris/core/error.h>
#include <iris/core/spinlock.h>

namespace di::sync {
class DumbSpinlock;
}

namespace di::platform {
using ThreadId = types::i32;

extern ThreadId get_current_thread_id();

using DefaultLock = iris::Spinlock;

template<typename T>
class DefaultFallibleAllocator {
public:
    using Value = T;

    constexpr iris::Expected<di::container::Allocation<T>> allocate(types::usize count) const {
        if consteval {
            return di::container::Allocator<T>().allocate(count);
        }
        auto* data = ::operator new(sizeof(T) * count,
                                    std::align_val_t { di::container::max(alignof(T), alignof(void*)) }, std::nothrow);
        if (!data) {
            return di::Unexpected(iris::Error::NotEnoughMemory);
        }
        return di::container::Allocation<T> { reinterpret_cast<T*>(data), count };
    }

    constexpr void deallocate(T* data, types::usize count) const {
        if consteval {
            return di::container::Allocator<T>().deallocate(data, count);
        }
        ::operator delete(data, sizeof(T) * count, std::align_val_t { di::container::max(alignof(T), alignof(void*)) });
    }
};

template<typename T>
using DefaultAllocator = DefaultFallibleAllocator<T>;

template<typename T>
using DefaultFallibleNewResult = vocab::Expected<T, iris::Error>;

constexpr iris::Error default_fallible_allocation_error() {
    return iris::Error::NotEnoughMemory;
}
}
