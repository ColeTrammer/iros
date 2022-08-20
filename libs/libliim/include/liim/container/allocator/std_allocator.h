#pragma once

#include <liim/forward.h>

#if !defined(__is_libc) && !defined(__is_libk)
#include <memory>
#else
#include <stdlib.h>

namespace std {
template<typename T>
struct allocator {
    using value_type = T;
    using size_type = LIIM::size_t;

    constexpr allocator() noexcept {}

    template<typename U>
    constexpr allocator(allocator<U> const&) noexcept {}

    constexpr ~allocator() {}

    [[nodiscard]] constexpr T* allocate(LIIM::size_t count) {
        if
            consteval {
                return static_cast<T*>(::operator new(count * sizeof(T)));
            }
        // FIXME: this should call ::operator new() once this polyfilled.
        if constexpr (alignof(T) > alignof(void*)) {
            return static_cast<T*>(aligned_alloc(alignof(T), count * sizeof(T)));
        }
        return static_cast<T*>(malloc(count * sizeof(T)));
    }

    constexpr void deallocate(T* pointer, LIIM::size_t) {
        if
            consteval {
                return ::operator delete(pointer);
            }
        // FIXME: this should call ::operator delete() once this polyfilled.
        free(pointer);
    }
};

template<class T, class U>
constexpr bool operator==(allocator<T> const& a, allocator<U> const& b) noexcept {
    return true;
}
}
#endif
