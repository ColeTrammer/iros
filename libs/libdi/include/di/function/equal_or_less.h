#pragma once

#include <di/concepts/implicitly_convertible_to.h>
#include <di/function/curry_back.h>

namespace di::function {
struct EqualOrLess {
    template<typename T, typename U>
    constexpr bool operator()(T const& a, U const& b) const
    requires(requires {
                 { a <= b } -> concepts::ImplicitlyConvertibleTo<bool>;
             })
    {
        return a <= b;
    }
};

constexpr inline auto equal_or_less = curry_back(EqualOrLess {}, meta::size_constant<2>);
}