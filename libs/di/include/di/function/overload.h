#pragma once

#include <di/function/piped.h>
#include <di/function/pipeline.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/util/forward.h>

namespace di::function {
namespace detail {
    template<typename... Funs>
    struct OverloadImpl : Funs... {
        using Funs::operator()...;
    };

    template<typename... Funs>
    OverloadImpl(Funs&&...) -> OverloadImpl<Funs...>;

    struct OverloadFunction {
        template<typename... Funs>
        requires(concepts::ConstructibleFrom<meta::Decay<Funs>, Funs> && ...)
        constexpr auto operator()(Funs&&... functions) const {
            return function::piped(OverloadImpl(util::forward<Funs>(functions)...));
        }
    };
}

constexpr inline auto overload = detail::OverloadFunction {};
}

namespace di {
using function::overload;
}
