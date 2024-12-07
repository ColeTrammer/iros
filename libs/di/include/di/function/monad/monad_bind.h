#pragma once

#include "di/function/tag_invoke.h"
#include "di/util/forward.h"

namespace di::function::monad {
struct BindFunction {
    template<typename T, typename F>
    constexpr auto operator()(T&& value, F&& function) const -> meta::TagInvokeResult<BindFunction, T, F> {
        return tag_invoke(*this, util::forward<T>(value), util::forward<F>(function));
    }
};

constexpr inline auto bind = BindFunction {};
}
