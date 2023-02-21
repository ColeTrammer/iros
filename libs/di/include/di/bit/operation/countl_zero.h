#pragma once

#include <di/concepts/unsigned_integer.h>
#include <di/math/numeric_limits.h>

namespace di::bit {
namespace detail {
    struct CountlZeroFunction {
        template<concepts::UnsignedInteger T>
        constexpr int operator()(T value) const {
            if (value == 0) {
                return math::NumericLimits<T>::digits;
            }

            if constexpr (sizeof(T) <= sizeof(unsigned int)) {
                constexpr auto extra_width = math::NumericLimits<unsigned int>::digits - math::NumericLimits<T>::digits;
                return __builtin_clz(value) - extra_width;
            } else if constexpr (sizeof(T) <= sizeof(unsigned long)) {
                return __builtin_clzl(value);
            } else if constexpr (sizeof(T) <= sizeof(unsigned long long)) {
                return __builtin_clzll(value);
            } else {
                static_assert(sizeof(T) == 16);
                auto high = u64(value >> 64);
                if (high != 0) {
                    return (*this)(high);
                }
                auto low = u64(value & math::NumericLimits<u64>::max);
                return (*this)(low) + 64;
            }
        }
    };
}

constexpr inline auto countl_zero = detail::CountlZeroFunction {};
}
