#pragma once

#include <di/util/forward.h>

namespace di::function {
struct Plus {
    template<typename T, typename U>
    requires(requires(T&& a, U&& b) { util::forward<T>(a) + util::forward<U>(b); })
    constexpr decltype(auto) operator()(T&& a, U&& b) const {
        return util::forward<T>(a) + util::forward<U>(b);
    }
};

constexpr inline auto plus = Plus {};
}