#pragma once

#include <di/function/curry_back.h>
#include <di/math/intcmp/cmp_less.h>
#include <di/meta/operations.h>

namespace di::function {
struct Less {
    template<typename T, typename U>
    constexpr auto operator()(T const& a, U const& b) const -> bool
    requires(requires {
        { a < b } -> concepts::ImplicitlyConvertibleTo<bool>;
    })
    {
        if constexpr (concepts::Integer<T> && concepts::Integer<U>) {
            return math::cmp_less(a, b);
        } else {
            return a < b;
        }
    }
};

constexpr inline auto less = curry_back(Less {}, meta::c_<2zu>);
}

namespace di {
using function::less;
using function::Less;
}
