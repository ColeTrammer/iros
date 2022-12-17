#pragma once

#include <di/parser/combinator/transform.h>

namespace di::parser {
namespace detail {
    struct IgnoreFunction {
        template<typename Parser>
        constexpr auto operator()(Parser&& parser) const {
            return transform(util::forward<Parser>(parser), [](auto&&...) {});
        }
    };
}

constexpr inline auto ignore = detail::IgnoreFunction {};
}