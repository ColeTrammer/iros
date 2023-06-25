#pragma once

#include <di/function/tag_invoke.h>
#include <di/meta/language.h>

namespace di::math {
namespace detail {
    struct AbsFunction {
        template<typename T>
        requires(concepts::TagInvocable<AbsFunction, T> || concepts::SignedIntegral<meta::RemoveCVRef<T>>)
        constexpr auto operator()(T&& value) const {
            if constexpr (concepts::TagInvocable<AbsFunction, T>) {
                return function::tag_invoke(*this, util::forward<T>(value));
            } else {
                return value < 0 ? -value : value;
            }
        }
    };
}

constexpr inline auto abs = detail::AbsFunction {};
}

namespace di {
using math::abs;
}
