#pragma once

#include <di/function/curry_back.h>
#include <di/math/intcmp/cmp_not_equal.h>
#include <di/meta/operations.h>

namespace di::function {
struct NotEqual {
    template<typename T, typename U>
    constexpr bool operator()(T const& a, U const& b) const
    requires(requires {
        { a != b } -> concepts::ImplicitlyConvertibleTo<bool>;
    })
    {
        if constexpr (concepts::Integer<T> && concepts::Integer<U>) {
            return math::cmp_not_equal(a, b);
        } else {
            return a != b;
        }
    }
};

constexpr inline auto not_equal = curry_back(NotEqual {}, meta::c_<2zu>);
}
