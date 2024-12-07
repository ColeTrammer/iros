#pragma once

#include "di/bit/operation/countl_zero.h"

namespace di::bit {
namespace detail {
    struct CountrOneFunction {
        template<concepts::UnsignedInteger T>
        constexpr auto operator()(T value) const -> int {
            return countr_zero(T(~value));
        }
    };
}

constexpr inline auto countr_one = detail::CountrOneFunction {};
}

namespace di {
using bit::countr_one;
}
