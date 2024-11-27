#pragma once

#include <di/function/curry_back.h>
#include <di/util/forward.h>

namespace di::function {
struct Multiplies {
    template<typename T, typename U>
    requires(requires(T&& a, U&& b) { util::forward<T>(a) * util::forward<U>(b); })
    constexpr auto operator()(T&& a, U&& b) const -> decltype(auto) {
        return util::forward<T>(a) * util::forward<U>(b);
    }
};

constexpr inline auto multiplies = function::curry_back(Multiplies {}, meta::c_<2zu>);
}

namespace di {
using function::multiplies;
using function::Multiplies;
}
