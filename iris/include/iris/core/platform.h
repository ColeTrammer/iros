#pragma once

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

template<typename T>
class DefaultAllocator {
public:
    using Value = T;

    constexpr di::container::Allocation<T> allocate(size_t count) const {
        if consteval {
            return di::container::Allocator<T>().allocate(count);
        } else {
            auto* data = ::operator new(sizeof(T) * count, std::align_val_t { di::max(alignof(T), alignof(void*)) }, std::nothrow);
            DI_ASSERT(data);

            // FIXME: propagate allocation failure, when di::TreeSet<> supports fallible allocation properly.
            // if (!data) {
            // return di::Unexpected(iris::Error::OutOfMemory);
            // }
            return di::container::Allocation<T> { reinterpret_cast<T*>(data), count };
        }
    }

    constexpr void deallocate(T* data, size_t count) const {
        if consteval {
            return di::container::Allocator<T>().deallocate(data, count);
        } else {
            ::operator delete(data, sizeof(T) * count, std::align_val_t { di::max(alignof(T), alignof(void*)) });
        }
    }
};
}