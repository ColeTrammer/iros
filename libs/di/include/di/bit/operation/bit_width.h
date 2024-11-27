#pragma once

#include <di/bit/operation/countl_zero.h>

namespace di::bit {
namespace detail {
    struct BitWidthFunction {
        template<concepts::UnsignedInteger T>
        constexpr auto operator()(T value) const -> T {
            return math::NumericLimits<T>::digits - countl_zero(value);
        }
    };
}

constexpr inline auto bit_width = detail::BitWidthFunction {};
}

namespace di {
using bit::bit_width;
}
