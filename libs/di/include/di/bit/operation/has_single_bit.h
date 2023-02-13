#pragma once

#include <di/bit/operation/popcount.h>

namespace di::bit {
namespace detail {
    struct HasSingleBitFunction {
        template<concepts::UnsignedInteger T>
        constexpr bool operator()(T value) const {
            return popcount(value) == 1;
        }
    };
}

constexpr inline auto has_single_bit = detail::HasSingleBitFunction {};
}