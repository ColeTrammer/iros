#pragma once

#include <di/bit/operation/bit_width.h>

namespace di::bit {
namespace detail {
    struct BitFloorFunction {
        template<concepts::UnsignedInteger T>
        constexpr T operator()(T value) const {
            if (value == 0) {
                return 0;
            }
            return T(1) << (bit_width(T(value - 1)));
        }
    };
}

constexpr inline auto bit_floor = detail::BitFloorFunction {};
}
