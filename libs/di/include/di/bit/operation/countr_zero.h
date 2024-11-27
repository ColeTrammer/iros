#pragma once

#include <di/math/numeric_limits.h>
#include <di/meta/language.h>

namespace di::bit {
namespace detail {
    struct CountrZeroFunction {
        template<concepts::UnsignedInteger T>
        constexpr auto operator()(T value) const -> int {
            if (value == 0) {
                return 0;
            }

            if constexpr (sizeof(T) <= sizeof(unsigned int)) {
                return __builtin_ctz(value);
            } else if constexpr (sizeof(T) <= sizeof(unsigned long)) {
                return __builtin_ctzl(value);
            } else if constexpr (sizeof(T) <= sizeof(unsigned long long)) {
                return __builtin_ctzll(value);
            } else {
                static_assert(sizeof(T) == 16);
                auto low = u64(value & math::NumericLimits<u64>::max);
                if (low != 0) {
                    return (*this)(low);
                }
                auto high = u64(value >> 64);
                return (*this)(high) + 64;
            }
        }
    };
}

constexpr inline auto countr_zero = detail::CountrZeroFunction {};
}

namespace di {
using bit::countr_zero;
}
