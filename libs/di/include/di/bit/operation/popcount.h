#pragma once

#include <di/concepts/unsigned_integer.h>
#include <di/math/numeric_limits.h>

namespace di::bit {
namespace detail {
    struct PopcountFunction {
        template<concepts::UnsignedInteger T>
        constexpr int operator()(T value) const {
            if constexpr (sizeof(T) <= sizeof(unsigned int)) {
                return __builtin_popcount(value);
            } else if constexpr (sizeof(T) <= sizeof(unsigned long)) {
                return __builtin_popcountl(value);
            } else if constexpr (sizeof(T) <= sizeof(unsigned long long)) {
                return __builtin_popcountll(value);
            } else {
                static_assert(sizeof(T) == 16);
                auto low = u64(value & math::NumericLimits<u64>::max);
                auto high = u64(value >> 64);
                return (*this)(low) + (*this)(high);
            }
        }
    };
}

constexpr inline auto popcount = detail::PopcountFunction {};
}
