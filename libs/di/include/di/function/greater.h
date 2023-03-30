#pragma once

#include <di/concepts/implicitly_convertible_to.h>
#include <di/function/curry_back.h>
#include <di/math/intcmp/cmp_greater.h>

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

constexpr inline auto greater = curry_back(Greater {}, meta::size_constant<2>);
}
