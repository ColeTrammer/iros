#pragma once

#include <di/function/curry_back.h>
#include <di/util/forward.h>

namespace di::function {
struct BitAnd {
    template<typename T, typename U>
    requires(requires(T&& a, U&& b) { util::forward<T>(a) & util::forward<U>(b); })
    constexpr decltype(auto) operator()(T&& a, U&& b) const {
        return util::forward<T>(a) & util::forward<U>(b);
    }
};

constexpr inline auto bit_and = function::curry_back(BitAnd {}, meta::c_<2zu>);
}

namespace di {
using function::bit_and;
using function::BitAnd;
}
