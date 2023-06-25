#pragma once

#include <di/function/curry_back.h>
#include <di/math/intcmp/cmp_greater.h>
#include <di/meta/operations.h>

namespace di::function {
struct Greater {
    template<typename T, typename U>
    constexpr bool operator()(T const& a, U const& b) const
    requires(requires {
        { a > b } -> concepts::ImplicitlyConvertibleTo<bool>;
    })
    {
        if constexpr (concepts::Integer<T> && concepts::Integer<U>) {
            return math::cmp_greater(a, b);
        } else {
            return a > b;
        }
    }
};

constexpr inline auto greater = curry_back(Greater {}, meta::c_<2zu>);
}

namespace di {
using function::greater;
using function::Greater;
}
