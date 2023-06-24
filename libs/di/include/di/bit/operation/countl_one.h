#pragma once

#include <di/bit/operation/countl_zero.h>

namespace di::bit {
namespace detail {
    struct CountlOneFunction {
        template<concepts::UnsignedInteger T>
        constexpr int operator()(T value) const {
            return countl_zero(T(~value));
        }
    };
}

constexpr inline auto countl_one = detail::CountlOneFunction {};
}

namespace di {
using bit::countl_one;
}
