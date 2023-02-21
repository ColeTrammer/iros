#pragma once

#include <di/function/tag_invoke.h>
#include <di/parser/concepts/parser_context.h>

namespace di::parser {
namespace detail {
    struct IntoParseContextFunction {
        template<typename T>
        requires(concepts::TagInvocable<IntoParseContextFunction, T> || concepts::ParserContext<T>)
        constexpr concepts::ParserContext decltype(auto) operator()(T&& value) const {
            if constexpr (concepts::TagInvocable<IntoParseContextFunction, T>) {
                return function::tag_invoke(*this, util::forward<T>(value));
            } else {
                return util::forward<T>(value);
            }
        }
    };
}

constexpr inline auto into_parser_context = detail::IntoParseContextFunction {};
}

namespace di::concepts {
template<typename T>
concept IntoParserContext = requires(T&& value) { parser::into_parser_context(util::forward<T>(value)); };
}

namespace di::meta {
template<concepts::IntoParserContext T>
using AsParserContext = decltype(parser::into_parser_context(util::declval<T>()));
}
