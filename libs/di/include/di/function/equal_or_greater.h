#pragma once

#include <di/function/curry_back.h>
#include <di/math/intcmp/cmp_greater_equal.h>
#include <di/meta/operations.h>

namespace di::function {
struct EqualOrGreater {
    template<typename T, typename U>
    constexpr bool operator()(T const& a, U const& b) const
    requires(requires {
        { a >= b } -> concepts::ImplicitlyConvertibleTo<bool>;
    })
    {
        if constexpr (concepts::Integer<T> && concepts::Integer<U>) {
            return math::cmp_greater_equal(a, b);
        } else {
            return a >= b;
        }
    }
};

constexpr inline auto equal_or_greater = curry_back(EqualOrGreater {}, meta::c_<2zu>);
}

namespace di {
using function::equal_or_greater;
using function::EqualOrGreater;
}
