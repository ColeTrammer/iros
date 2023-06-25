#pragma once

#include <di/function/tag_invoke.h>
#include <di/vocab/error/concepts/status_code.h>

namespace di::vocab {
namespace detail {
    struct IntoStatusCodeFunction {
        template<typename... Args>
        requires(concepts::TagInvocable<IntoStatusCodeFunction, Args...>)
        constexpr concepts::StatusCode decltype(auto) operator()(Args&&... args) const {
            return function::tag_invoke(*this, util::forward<Args>(args)...);
        }
    };
}

constexpr inline auto into_status_code = detail::IntoStatusCodeFunction {};
}

namespace di::concepts {
template<typename... Args>
concept ConvertibleToAnyStatusCode =
    requires(Args&&... args) { vocab::into_status_code(util::forward<Args>(args)...); };

template<typename Result, typename... Args>
concept ConvertibleToStatusCode =
    requires(Args&&... args) { Result(vocab::into_status_code(util::forward<Args>(args)...)); };
}

namespace di {
using vocab::into_status_code;
}
