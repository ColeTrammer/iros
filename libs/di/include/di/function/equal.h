#pragma once

#include <di/function/curry_back.h>
#include <di/math/intcmp/cmp_equal.h>
#include <di/meta/operations.h>

namespace di::function {
struct Equal {
    template<typename T, typename U>
    constexpr auto operator()(T const& a, U const& b) const -> bool
    requires(requires {
        { a == b } -> concepts::ImplicitlyConvertibleTo<bool>;
    })
    {
        if constexpr (concepts::Integral<T> && concepts::Integral<U>) {
            return math::cmp_equal(a, b);
        } else {
            return a == b;
        }
    }
};

constexpr inline auto equal = curry_back(Equal {}, meta::c_<2ZU>);
}

namespace di {
using function::equal;
using function::Equal;
}
