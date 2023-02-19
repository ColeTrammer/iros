#pragma once

#include <di/container/algorithm/max.h>
#include <di/container/allocator/forward_declaration.h>
#include <di/types/integers.h>
#include <di/util/std_new.h>
#include <iris/core/error.h>

namespace di::sync {
class DumbSpinlock;
}

namespace di::platform {
using ThreadId = types::i32;

extern ThreadId get_current_thread_id();

using DefaultLock = sync::DumbSpinlock;

// FIXME: make the default allocator fallible as well
//        once di::TreeSet<> supports fallible allocation.
template<typename T>
struct DefaultAllocator {
public:
    using Value = T;

    constexpr di::container::Allocation<T> allocate(types::usize count) const {
        if consteval {
            return di::container::Allocator<T>().allocate(count);
        } else {
            auto* data = ::operator new(
                sizeof(T) * count, std::align_val_t { di::container::max(alignof(T), alignof(void*)) }, std::nothrow);
            DI_ASSERT(data);
            return di::container::Allocation<T> { reinterpret_cast<T*>(data), count };
        }
    }

    constexpr void deallocate(T* data, types::usize count) const {
        if consteval {
            return di::container::Allocator<T>().deallocate(data, count);
        } else {
            ::operator delete(data, sizeof(T) * count,
                              std::align_val_t { di::container::max(alignof(T), alignof(void*)) });
        }
    }
};

template<typename T>
class DefaultFallibleAllocator {
public:
    using Value = T;

    constexpr iris::Expected<di::container::Allocation<T>> allocate(types::usize count) const {
        if consteval {
            return di::container::Allocator<T>().allocate(count);
        } else {
            auto* data = ::operator new(
                sizeof(T) * count, std::align_val_t { di::container::max(alignof(T), alignof(void*)) }, std::nothrow);
            if (!data) {
                return di::Unexpected(iris::Error::OutOfMemory);
            }
            return di::container::Allocation<T> { reinterpret_cast<T*>(data), count };
        }
    }

    constexpr void deallocate(T* data, types::usize count) const {
        if consteval {
            return di::container::Allocator<T>().deallocate(data, count);
        } else {
            ::operator delete(data, sizeof(T) * count,
                              std::align_val_t { di::container::max(alignof(T), alignof(void*)) });
        }
    }
};
}