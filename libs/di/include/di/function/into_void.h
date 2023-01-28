#pragma once

namespace di::function {
namespace detail {
    struct IntoVoidFunction {
        template<typename... Args>
        constexpr void operator()(Args&&...) const {}
    };
}

constexpr inline auto into_void = detail::IntoVoidFunction {};
}