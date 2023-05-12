#pragma once

#include <di/function/tag_invoke.h>
#include <di/util/forward.h>

namespace di::execution {
namespace as_awaitable_ns {
    struct Function;
}

extern as_awaitable_ns::Function const as_awaitable;

template<typename Derived>
struct WithAwaitTransform {
    template<typename T>
    T&& await_transform(T&& value) noexcept {
        return util::forward<T>(value);
    }

    template<typename T>
    requires(concepts::TagInvocable<as_awaitable_ns::Function, T, Derived&>)
    auto await_transform(T&& value) noexcept -> meta::TagInvokeResult<as_awaitable_ns::Function, T, Derived&> {
        return function::tag_invoke(as_awaitable, util::forward<T>(value), static_cast<Derived&>(*this));
    }
};
}
