#pragma once

#include <di/function/curry_back.h>
#include <di/util/forward.h>

namespace di::function {
struct Minus {
    template<typename T, typename U>
    requires(requires(T&& a, U&& b) { util::forward<T>(a) - util::forward<U>(b); })
    constexpr decltype(auto) operator()(T&& a, U&& b) const {
        return util::forward<T>(a) - util::forward<U>(b);
    }
};

constexpr inline auto minus = function::curry_back(Minus {}, meta::c_<2zu>);
}
