#pragma once

#include <di/concepts/implicitly_convertible_to.h>

namespace di::function {
struct NotEqual {
    template<typename T, typename U>
    constexpr bool operator()(T const& a, U const& b) const
    requires(requires {
                 { a != b } -> concepts::ImplicitlyConvertibleTo<bool>;
             })
    {
        return a != b;
    }
};

constexpr inline auto not_equal = NotEqual {};
}