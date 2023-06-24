#pragma once

#include <di/function/curry_back.h>
#include <di/math/intcmp/cmp_less_equal.h>
#include <di/meta/operations.h>

namespace di::function {
struct EqualOrLess {
    template<typename T, typename U>
    constexpr bool operator()(T const& a, U const& b) const
    requires(requires {
        { a <= b } -> concepts::ImplicitlyConvertibleTo<bool>;
    })
    {
        if constexpr (concepts::Integer<T> && concepts::Integer<U>) {
            return math::cmp_less_equal(a, b);
        } else {
            return a <= b;
        }
    }
};

constexpr inline auto equal_or_less = curry_back(EqualOrLess {}, meta::c_<2zu>);
}
