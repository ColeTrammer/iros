#pragma once

#include <di/bit/operation/bit_width.h>
#include <di/concepts/same_as.h>

namespace di::bit {
namespace detail {
    struct BitCeilFunction {
        template<concepts::UnsignedInteger T>
        constexpr T operator()(T value) const {
            if (value <= 1u) {
                return T(1);
            }
            if constexpr (concepts::SameAs<T, decltype(+value)>) {
                return T(1) << bit_width(T(value - 1));
            } else {
                // Ensure UB if the result cannot fit inside T. This is needed because
                // T promotes to an int automatically.
                constexpr int extra_offset = math::NumericLimits<unsigned int>::digits - math::NumericLimits<T>::digits;
                return T(1u << (bit_width(T(value - 1)) + extra_offset) >> extra_offset);
            }
        }
    };
}

constexpr inline auto bit_ceil = detail::BitCeilFunction {};
}
