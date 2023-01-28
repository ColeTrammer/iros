#pragma once

#include <di/types/ptrdiff_t.h>
#include <di/types/size_t.h>

#ifdef DI_USE_STD
#include <memory>
#else
#include <di/util/std_new.h>

namespace std {
template<typename T, T v>
struct integral_constant {
    constexpr static T value = v;
    using value_type = T;
    using type = integral_constant;
    constexpr operator T() const noexcept { return value; }
    constexpr T operator()() const noexcept { return value; }
};

template<typename T>
struct allocator {
    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using propagate_on_container_move_assignment = integral_constant<bool, true>;

    constexpr allocator() noexcept {}

    template<typename U>
    constexpr allocator(allocator<U> const&) noexcept {}

    constexpr ~allocator() {}

    [[nodiscard]] constexpr T* allocate(size_t count) {
        if consteval {
            return static_cast<T*>(::operator new(count * sizeof(T)));
        }
        if constexpr (alignof(T) > alignof(void*)) {
            return static_cast<T*>(::operator new(count * sizeof(T), std::align_val_t { alignof(T) }));
        } else {
            return static_cast<T*>(::operator new(count * sizeof(T)));
        }
    }

    constexpr void deallocate(T* pointer, size_t count) {
        if consteval {
            return ::operator delete(pointer);
        }
        if constexpr (alignof(T) > alignof(void*)) {
            return ::operator delete(pointer, count * sizeof(T), std::align_val_t { alignof(T) });
        } else {
            return ::operator delete(pointer, count * sizeof(T));
        }
    }
};

template<class T, class U>
constexpr bool operator==(allocator<T> const& a, allocator<U> const& b) noexcept {
    return true;
}
}
#endif

namespace di::container {
template<typename T>
using StdAllocator = std::allocator<T>;
}
