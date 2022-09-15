#pragma once

#include <di/math/numeric_limits.h>
#include <di/types/prelude.h>
#include <di/function/invoke.h>

namespace di::math {
namespace detail {
    template<size_t size>
    constexpr auto smallest_unsigned_type_helper(InPlaceIndex<size>) {
        if constexpr (size <= NumericLimits<unsigned char>::max) {
            return (unsigned char) 0;
        } else if constexpr (size <= NumericLimits<unsigned short>::max) {
            return (unsigned short) 0;
        } else if constexpr (size <= NumericLimits<unsigned int>::max) {
            return (unsigned int) 0;
        } else if constexpr (size <= NumericLimits<unsigned long>::max) {
            return (unsigned long) 0;
        } else {
            return (unsigned long long) 0;
        }
    }
}

template<size_t size>
using SmallestUnsignedType = decltype(detail::smallest_unsigned_type_helper(in_place_index<size>));
}