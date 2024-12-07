#pragma once

#include "di/bit/operation/popcount.h"

namespace di::bit {
namespace detail {
    struct HasSingleBitFunction {
        template<concepts::UnsignedInteger T>
        constexpr auto operator()(T value) const -> bool {
            return popcount(value) == 1;
        }
    };
}

constexpr inline auto has_single_bit = detail::HasSingleBitFunction {};
}

namespace di {
using bit::has_single_bit;
}
