#pragma once

#include <di/container/allocator/allocation.h>
#include <di/container/allocator/allocator.h>
#include <di/container/allocator/std_allocator.h>
#include <di/types/prelude.h>
#include <di/util/std_new.h>
#include <di/vocab/error/generic_domain.h>
#include <di/vocab/expected/prelude.h>

namespace di::container {
template<typename T>
class FallibleAllocator {
public:
    using Value = T;

    constexpr Expected<Allocation<T>, vocab::GenericCode> allocate(size_t count) const {
        if consteval {
            return Allocator<T>().allocate(count);
        } else {
            auto* data = ::operator new(
                sizeof(T) * count, std::align_val_t { di::container::max(alignof(T), alignof(void*)) }, std::nothrow);
            if (!data) {
                return vocab::Unexpected(vocab::BasicError::FailedAllocation);
            }
            return Allocation<T> { static_cast<T*>(data), count };
        }
    }

    constexpr void deallocate(T* data, size_t count) const {
        if consteval {
            return std::allocator<T>().deallocate(data, count);
        } else {
            ::operator delete(data, sizeof(T) * count,
                              std::align_val_t { di::container::max(alignof(T), alignof(void*)) });
        }
    }
};
}
