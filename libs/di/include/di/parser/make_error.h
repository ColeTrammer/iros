#pragma once

#include "di/parser/concepts/parser_context.h"

namespace di::parser {
namespace detail {
    struct MakeErrorFunction {
        template<typename... Args>
        constexpr auto operator()(concepts::ParserContext auto& context, Args&&... args) const {
            if constexpr (requires { context.make_error(util::forward<Args>(args)...); }) {
                return context.make_error(util::forward<Args>(args)...);
            } else {
                return context.make_error();
            }
        }
    };
}

constexpr inline auto make_error = detail::MakeErrorFunction {};
}
