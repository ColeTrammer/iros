#pragma once

#include <di/function/invoke.h>
#include <di/math/numeric_limits.h>
#include <di/types/prelude.h>

namespace di::math {
namespace detail {
    template<umax size>
    constexpr auto smallest_unsigned_type_helper(InPlaceValue<size>) {
        if constexpr (size <= NumericLimits<u8>::max) {
            return (u8) 0;
        } else if constexpr (size <= NumericLimits<u16>::max) {
            return (u16) 0;
        } else if constexpr (size <= NumericLimits<u32>::max) {
            return (u32) 0;
        } else if constexpr (size <= NumericLimits<u64>::max) {
            return (u64) 0;
        } else {
            return (umax) 0;
        }
    }
}

template<umax size>
using SmallestUnsignedType = decltype(detail::smallest_unsigned_type_helper(in_place_value<size>));
}