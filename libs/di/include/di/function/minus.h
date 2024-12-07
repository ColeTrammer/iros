#pragma once

#include "di/function/curry_back.h"
#include "di/util/forward.h"

namespace di::function {
struct Minus {
    template<typename T, typename U>
    requires(requires(T&& a, U&& b) { util::forward<T>(a) - util::forward<U>(b); })
    constexpr auto operator()(T&& a, U&& b) const -> decltype(auto) {
        return util::forward<T>(a) - util::forward<U>(b);
    }
};

constexpr inline auto minus = function::curry_back(Minus {}, meta::c_<2ZU>);
}

namespace di {
using function::minus;
using function::Minus;
}
